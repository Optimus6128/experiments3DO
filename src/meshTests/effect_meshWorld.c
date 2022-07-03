#include "core.h"

#include "effect_meshWorld.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"

#include "sprite_engine.h"


static Mesh *gridMesh;
static Mesh *cubeMesh;
static Object3D *gridObj;
static Object3D *cubeObj;

static Texture *gridTex;
static Texture *cubeTex;
static uint16 gridPal[32];
static uint16 cubePal[32];

static int rotX=8, rotY=62, rotZ=0;
static int zoom=320;

static int rotVel = 2;
static int zoomVel = 2;

void effectMeshWorldInit()
{
	MeshgenParams gridParams = makeMeshgenGridParams(1024, 16);
	MeshgenParams cubeParams = DEFAULT_MESHGEN_PARAMS(128);

	setPalGradient(0,31, 1,1,3, 27,29,31, gridPal);
	setPalGradient(0,31, 15,7,3, 19,11,23, cubePal);

	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, false, NULL);
	cubeTex = initGenTexture(64,64, 8, cubePal, 1, TEXGEN_CLOUDS, false, NULL);

	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT, gridTex);
	gridObj = initObject3D(gridMesh);

	cubeMesh = initGenMesh(MESH_CUBE, cubeParams, MESH_OPTIONS_DEFAULT, cubeTex);
	cubeObj = initObject3D(cubeMesh);
}

static void inputScript()
{
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		rotX += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		rotX -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		rotY += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		rotY -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		rotZ += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		rotZ -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		zoom += zoomVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		zoom -= zoomVel;
	}
}

void effectMeshWorldRun()
{
	inputScript();

	setObject3Dpos(gridObj, 0, -64, zoom);
	setObject3Drot(gridObj, rotX, rotY, rotZ);

	setObject3Dpos(cubeObj, 0, 64, zoom);
	setObject3Drot(cubeObj, rotX, rotY, rotZ);

	renderObject3D(gridObj);
	renderObject3D(cubeObj);

	drawNumber(8,192, rotX);
	drawNumber(8,200, rotY);
	drawNumber(8,208, rotZ);
	drawNumber(8,216, zoom);
}
