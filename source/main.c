/**
* Hello Triangle example, made by Lectem
* Thanks to smea, fincs, neobrain, xerpi and all those who helped me understand how the 3DS GPU works
*/

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpuframework.h"

/**
 * Vertex shader input components that aren't specified by data in the vertex
 * array (because it has less than 4 components, like the color attribute in
 * this test) take on a default value of (_, 0.0, 0.0, 1.0) for the missing
 * components. (At least one component is required, so the the .x component
 * can't have a default.) Specifically, unspecified components are *not* taken
 * from a "default attribute" specified for the vertex: if there's an array
 * bound to the attribute, the "default attribute" is completely ignored.
 *
 * The test renders two triangles, specifying only .xyz components of the
 * position and .xy components of the color. The vertex shader then uses all 4
 * input components of both input attributes to produce the output. Due to the
 * default attribute values, position.w takes on to 1.0 and color.zw take on
 * 0.0 and 1.0. The result is one triangle is rendered in yellow fading to dark
 * yellow (produced by e.g. the input [1, 1, _, _] being completed with 0.0,
 * giving [1, 1, 0]) and another with one vertex each in pure red, pure green
 * and black (again, due to the blue attribute taking the value 0).  To verify
 * the same also works for .w, you can press the A button to make the shader
 * instead set the output color to incolor.xyw. This should produce blue or
 * bluish triangles.
 *
 * "Default" attribute values of [0.5, 0.5, 0.5, 0.5] are specified for the
 * color attribute. They are not used by the hardware, otherwise using .z or .w
 * would yield the same slightly bluish triangles, which is not the case.
 */

typedef struct Vertex {
	float pos[3];
	float texcoord[2];
} Vertex;

static const Vertex test_mesh[] = {
	// Top-right triangle
	{{  0.0f,   0.0f, 0.5f}, {0.0f, 0.0f}},
	{{400.0f,   0.0f, 0.5f}, {1.0f, 0.0f}},
	{{400.0f, 240.0f, 0.5f}, {1.0f, 1.0f}},

	// Bottom-left triangle
	{{400.0f, 240.0f, 0.5f}, {1.0f, 1.0f}},
	{{  0.0f, 240.0f, 0.5f}, {0.0f, 1.0f}},
	{{  0.0f,   0.0f, 0.5f}, {0.0f, 0.0f}},
};

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

// Just a re-implementation of the equivalent ctrulib function but with a nice interface
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
	void* test_data = linearAlloc(sizeof(test_mesh));		//allocate our vbo on the linear heap
	memcpy(test_data, test_mesh, sizeof(test_mesh)); //Copy our data

	VertexAttributeConfig attribute_config = {
		.default_attribute_mask = 0,
		.num_attributes = 2,
		.num_arrays = 1,
		.array_address_base = osConvertVirtToPhys((uintptr_t)test_data),
		.attribute_types =
			GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | // float pos[3]
			GPU_ATTRIBFMT(1, 2, GPU_FLOAT),  // float texcoord[2]
		.arrays = {
			{ .address_offset = 0, .attribute_map0 = 0x10, .stride = sizeof(Vertex), .num_attributes = 2 },
		},
	};

	size_t texture_size = 64 * 64 * sizeof(u16);
	u16* texture_data = linearAlloc(texture_size);

	u16* buffer = malloc(texture_size);
	for (int y = 0; y < 64; ++y) {
		for (int x = 0; x < 64; ++x) {
			buffer[y * 64 + x] = ((y << 2 | y >> 6) & 0xFF) << 8 | ((x << 2 | x >> 6) & 0xFF);
		}
	}

	copyTextureAndTile16(texture_data, buffer, 64, 64);
	free(buffer);

	do {
		hidScanInput();
		u32 keys = keysDown();
		if (keys & KEY_START) break; //Stop the program when Start is pressed

		gpuStartFrame();

		// Configure attribute arrays
		GPUY_ConfigureAttributes(&attribute_config);
		// Configure attribute -> shader input mappings
		GPUCMD_AddWrite(0x2bb, 0x00000010);
		GPUCMD_AddWrite(0x2bc, 0x00000000);

		GPU_SetTextureEnable(GPU_TEXUNIT0);

		GPU_SetTexture(GPU_TEXUNIT0, (u32*)osConvertVirtToPhys((u32)texture_data),
				64, 64,
				GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR) |
				GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_EDGE) | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_EDGE),
				GPU_HILO8);

		int texenvnum=0;
		GPU_SetTexEnv(
				texenvnum,
				GPU_TEVSOURCES(GPU_TEXTURE0, 0, 0),
				GPU_TEVSOURCES(GPU_TEXTURE0, 0, 0),
				GPU_TEVOPERANDS(0, 0, 0),
				GPU_TEVOPERANDS(0, 0, 0),
				GPU_REPLACE, GPU_REPLACE,
				0xAABBCCDD
		);

		//Display the buffers data
		GPU_DrawArray(GPU_TRIANGLES, sizeof(test_mesh) / sizeof(test_mesh[0]));

		gpuEndFrame();
	} while(aptMainLoop());

	linearFree(texture_data);
	linearFree(test_data);

	gpuUIExit();

	gfxExit();
	sdmcExit();
	hidExit();
	aptExit();
	srvExit();

	return 0;
}
