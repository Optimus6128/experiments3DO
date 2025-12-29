#include <math.h>

#include "core.h"

#include "system_graphics.h"
#include "tools.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"

#include "mathutil.h"
#include "input.h"

#include "raytrace.h"

static Mesh *cubeMesh;
static Object3D *cubeObj;
static Texture *cubeTex;

static Camera *camera;

static int rtUpdateIndex = 0;
static int buffToRender = 1;


void effectRaytraceInit()
{
	loadAndSetBackgroundImage("data/background.img", getBackBuffer());

	cubeTex = initTextures(RT_WIDTH, RT_HEIGHT, 16, TEXTURE_TYPE_DEFAULT, NULL, NULL, 0, 2);
	cubeMesh = initGenMesh(MESH_CUBE, DEFAULT_MESHGEN_PARAMS(256), MESH_OPTIONS_DEFAULT, cubeTex);
	cubeObj = initObject3D(cubeMesh);

	camera = createCamera();

	updateAllPolyTextureId(cubeMesh, buffToRender);

	raytraceInit();
}

void effectRaytraceRun()
{
	static int tr = 0;
	int t = getTicks();

	raytraceRun((uint16*)cubeTex[buffToRender].bitmap, rtUpdateIndex, tr>>1);

	setObject3Dpos(cubeObj, 0, 0, 640);
	setObject3Drot(cubeObj, t>>5, t>>6, t>>7);
	renderObject3D(cubeObj, camera, NULL, 0);

	rtUpdateIndex = (rtUpdateIndex + 1) & (RT_UPDATE_PIECES - 1);
	if (rtUpdateIndex == 0) {
		updateAllPolyTextureId(cubeMesh, buffToRender);
		buffToRender ^= 1;
		tr = getTicks();
	}
}
