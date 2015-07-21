/**
* Hello Triangle example, made by Lectem
*
* Draws a white triangle using the 3DS GPU.
* This example should give you enough hints and links on how to use the GPU for basic non-textured rendering.
* Another version of this example will be made with colors.
*
* Thanks to smea, fincs, neobrain, xerpi and all those who helped me understand how the 3DS GPU works
*/


#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpuframework.h"

#define COLOR_WHITE {0xFF,0xFF,0xFF,0xFF}
#define COLOR_RED	{0xFF,0x00,0x00,0xFF}
#define COLOR_GREEN {0x00,0xFF,0x00,0xFF}
#define COLOR_BLUE	{0x00,0x00,0xFF,0xFF}

typedef struct Vertex {
	float pos[3];
	float texcoord[2];
	u8 color[2];
} Vertex;

//Our data
static const Vertex test_mesh[] = {
	{{  0.0f,   0.0f, 0.5f}, {0.0f, 0.0f}, {0xFF,0xFF,/*0xFF,0xFF*/}},
	{{400.0f,   0.0f, 0.5f}, {1.0f, 0.0f}, {0x64,0x64,/*0x64,0xFF*/}},
	{{400.0f, 240.0f, 0.5f}, {1.0f, 1.0f}, {0xFF,0xFF,/*0xFF,0xFF*/}},

	{{400.0f, 240.0f, 0.5f}, {1.0f, 1.0f}, {0xFF,0x00,/*0x00,0xFF*/}},
	{{  0.0f, 240.0f, 0.5f}, {0.0f, 1.0f}, {0x00,0xFF,/*0x00,0xFF*/}},
	{{  0.0f,   0.0f, 0.5f}, {0.0f, 0.0f}, {0x00,0x00,/*0xFF,0xFF*/}}
};

static void* test_data = NULL;

static u32* test_texture=NULL;
static const u16 test_texture_w=256;
static const u16 test_texture_h=256;

extern const struct {
  u32	 width;
  u32	 height;
  u32	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  u8	 pixel_data[256 * 256 * 4 + 1];
} texture_data;

typedef struct VertexArrayConfig {
	u32 address_offset;
	u32 attribute_map0;
	u16 attribute_map1;
	u8 stride;
	u8 num_attributes;
} VertexArrayConfig;

typedef struct VertexAttributeConfig {
	u16 default_attribute_mask;
	u8 num_attributes;
	u8 num_arrays;
	uintptr_t array_address_base;
	u64 attribute_types;
	VertexArrayConfig arrays[12];
} VertexAttributeConfig;

void GPUY_ConfigureAttributes(const VertexAttributeConfig* config) {
	u32 buffer[39];

	buffer[0] = // 0x200
			config->array_address_base >> 3;
	buffer[1] = // 0x201
			config->attribute_types & 0xFFFFFFFF;
	buffer[2] = // 0x202
			((config->attribute_types >> 32) & 0xFFFF) |
			(config->default_attribute_mask << 16) |
			((config->num_attributes - 1) << 28);

	for (u8 i = 0; i < config->num_arrays; ++i) {
		const VertexArrayConfig* array = &config->arrays[i];
		int base = 3 + i*3;

		buffer[base+0] = // 0x203 ...
				array->address_offset;
		buffer[base+1] = // 0x204 ...
				array->attribute_map0;
		buffer[base+2] = // 0x205 ...
				array->attribute_map1 |
				(array->stride << 16) |
				(array->num_attributes << 28);
	}
	memset(&buffer[3 + config->num_arrays * 3], 0, (12 - config->num_arrays) * 3 * sizeof(u32));

	GPUCMD_AddIncrementalWrites(0x200, buffer, 39);

	GPUCMD_AddMaskedWrite(0x2b9, 0xB, 0xA0000000 | (config->num_attributes - 1));
	GPUCMD_AddMaskedWrite(0x242, 0x1, config->num_attributes - 1);
}

static u32 f24_from_float(float f) {
	if (!f)
		return 0;

	u32 v;
	memcpy(&v, &f, sizeof(v));

	u8 s = v >> 31;
	u32 exp = ((v >> 23) & 0xFF) - 0x40;
	u32 man = (v >> 7) & 0xFFFF;

	if (exp >= 0)
		return man | (exp << 16) | (s << 23);
	else
		return s << 23;
}

void GPUY_PackFourFloatIntoF24(const float input[4], u32 output[3]) {
	u32 tmp[4] = {
		f24_from_float(input[3]),
		f24_from_float(input[2]),
		f24_from_float(input[1]),
		f24_from_float(input[0]),
	};

	output[0] = tmp[0] <<  8 | tmp[1] >> 16;
	output[1] = tmp[1] << 16 | tmp[2] >>  8;
	output[2] = tmp[2] << 24 | tmp[3];
}

void GPUY_SetDefaultAttribute(int attribute_index, float components[4]) {
	u32 buffer[3];
	GPUY_PackFourFloatIntoF24(components, buffer);

	GPUCMD_AddMaskedWrite(0x232, 0x1, attribute_index);
	GPUCMD_AddIncrementalWrites(0x233, buffer, 3);
}

int main(int argc, char** argv)
{
	srvInit();
	aptInit();
	hidInit(NULL);
	sdmcInit();

	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	gpuUIInit();

	printf("hello triangle !\n");
	test_data = linearAlloc(sizeof(test_mesh));		//allocate our vbo on the linear heap
	memcpy(test_data, test_mesh, sizeof(test_mesh)); //Copy our data

	VertexAttributeConfig attribute_config = {
		.default_attribute_mask = 0x004,
		.num_attributes = 3,
		.num_arrays = 1,
		.array_address_base = osConvertVirtToPhys((uintptr_t)test_data),
		.attribute_types =
			GPU_ATTRIBFMT(0, 3, GPU_FLOAT) |
			GPU_ATTRIBFMT(1, 2, GPU_FLOAT) |
			GPU_ATTRIBFMT(2, 2, GPU_UNSIGNED_BYTE),
		.arrays = {
			{ .address_offset = 0, .attribute_map0 = 0x210, .stride = sizeof(Vertex), .num_attributes = 3 },
		},
	};

	//Allocate a RGBA8 texture with dimensions of 1x1
	test_texture = linearMemAlign(test_texture_w*test_texture_h*sizeof(u32),0x80);

	copyTextureAndTile((u8*)test_texture,texture_data.pixel_data,texture_data.width ,texture_data.height);
	
	if(!test_texture)printf("couldn't allocate test_texture\n");
	do{
		hidScanInput();
		u32 keys = keysDown();
		if(keys&KEY_START)break; //Stop the program when Start is pressed


		gpuStartFrame();

		//Setup the buffers data
		GPUY_ConfigureAttributes(&attribute_config);
		GPUCMD_AddWrite(0x2bb, 0x00000120);
		GPUCMD_AddWrite(0x2bc, 0x00000000);

		float tmp[4] = {127.0f, 127.0f, 255.0f, 255.0f};
		GPUY_SetDefaultAttribute(2, tmp);

		GPU_SetTextureEnable(GPU_TEXUNIT0);

		GPU_SetTexture(
				GPU_TEXUNIT0,
				(u32 *)osConvertVirtToPhys((u32) test_texture),
				// width and height swapped?
				test_texture_h,
				test_texture_w,
				GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR) |
				GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_EDGE) | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_EDGE),
				GPU_RGBA8
		);
//		  GPUCMD_AddWrite(GPUREG_0081, 0xFFFF0000);
		int texenvnum=0;
		GPU_SetTexEnv(
				texenvnum,
				GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_TEXTURE0, 0),
				GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_TEXTURE0, 0),
				GPU_TEVOPERANDS(0, 0, 0),
				GPU_TEVOPERANDS(0, 0, 0),
				GPU_MODULATE, GPU_MODULATE,
				0xAABBCCDD
		);

		//Display the buffers data
		GPU_DrawArray(GPU_TRIANGLES, sizeof(test_mesh) / sizeof(test_mesh[0]));

		gpuEndFrame();
	}while(aptMainLoop() );


	if(test_data)
	{
		linearFree(test_data);
	}
	if(test_texture)
	{
		linearFree(test_texture);
	}

	gpuUIExit();


	gfxExit();
	sdmcExit();
	hidExit();
	aptExit();
	srvExit();

	return 0;
}
