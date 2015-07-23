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
 * Vertex color saturation test.
 *
 * The PICA drops the sign and saturates to 1.0 the output color values from
 * the vertex shader. This happens on the vertex shader output, *not* after
 * interpolation. I suspect this is because internally the vertex shader output
 * is converted to a fixed-point number with no integer or sign bits, which is
 * then used for interpolation.
 *
 * The test involves rendering triangles with negative or >1.0 vertex colors,
 * and verifying that they interpolate as if the values were saturated as
 * above.
 *
 * The top-left triangle has colors [1, 0, 0] (pure red), [1, 1, 1] (pure
 * white) and [-1.5, -1.5, -1.5]. If the above didn't happen, you would see the
 * triangle interpolating through to black before going into negative and
 * having the color value clamped to 0. Instead however, the colors go smoothly
 * from red to pure white, and the vertices with colors [1,1,1] and
 * [-1.5,-1.5,-1.5] in fact have the same color.
 *
 * The bottom-left triangle shows a similar result, the [-1.5, -1.5, -1.5] and
 * [1.5, 1.5, 1.5] vertices both appear as pure white with no color change when
 * interpolating between them, and the [-0.5, 0, 0] vertex appears as a darker
 * red.
 */

struct Vertex {
	float pos[3];
	float color[4];
};

static const struct Vertex test_mesh[] = {
	// Top-Right triangle
	{{0.0f,   0.0f,   0.5f}, { 1.0f, 0.0f, 0.0f, 1.0f}},
	{{400.0f, 0.0f,   0.5f}, { 1.0f, 1.0f, 1.0f, 1.0f}},
	{{400.0f, 240.0f, 0.5f}, {-1.5f,-1.5f,-1.5f, 1.0f}},

	// Bottom-left triangle
	{{400.0f, 240.0f, 0.5f}, {-0.5f, 0.0f, 0.0f, 1.0f}},
	{{0.0f,   240.0f, 0.5f}, { 1.5f, 1.5f, 1.5f, 1.0f}},
	{{0.0f,   0.0f,   0.5f}, {-1.5f,-1.5f,-1.5f, 1.0f}}
};

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

	do{
		hidScanInput();
		u32 keys = keysDown();
		if(keys&KEY_START)break; //Stop the program when Start is pressed

		gpuStartFrame();

		//Setup the buffers data
		GPU_SetAttributeBuffers(
				2, // number of attributes
				(u32 *) osConvertVirtToPhys((u32) test_data),

				// Attributes
				GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | // float pos[3]
				GPU_ATTRIBFMT(1, 4, GPU_FLOAT), // float color[4]

				0x000,//Attribute mask, in our case 0b1110 since we use only the first one
				0x10,//Attribute permutations (here it is the identity)
				1, //number of buffers
				(u32[]) {0x0}, // buffer offsets (placeholders)
				(u64[]) {0x10}, // attribute permutations for each buffer (identity again)
				(u8[]) {2} // number of attributes for each buffer
		);

		int texenvnum=0;
		GPU_SetTexEnv(
				texenvnum,
				GPU_TEVSOURCES(GPU_PRIMARY_COLOR, 0, 0),
				GPU_TEVSOURCES(GPU_PRIMARY_COLOR, 0, 0),
				GPU_TEVOPERANDS(0, 0, 0),
				GPU_TEVOPERANDS(0, 0, 0),
				GPU_REPLACE, GPU_REPLACE,
				0xAABBCCDD
		);

		//Display the buffers data
		GPU_DrawArray(GPU_TRIANGLES, sizeof(test_mesh) / sizeof(test_mesh[0]));

		gpuEndFrame();
	}while(aptMainLoop() );


	linearFree(test_data);

	gpuUIExit();


	gfxExit();
	sdmcExit();
	hidExit();
	aptExit();
	srvExit();

	return 0;
}
