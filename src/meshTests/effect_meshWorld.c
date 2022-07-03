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

static Camera *camera;

static Mesh *gridMesh;
static Mesh *cubeMesh;
static Object3D *gridObj;
static Object3D *cubeObj;

static Texture *gridTex;
static Texture *cubeTex;
static uint16 gridPal[32];
static uint16 cubePal[32];

static int camRotX = 0;
static int camRotY = 0;
static int camRotZ = 0;
static int camZoom = 0;

static int camRotVel = 2;
static int camZoomVel = 2;

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

	camera = createCamera();
}

static void inputScript()
{
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		camRotX += camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		camRotX -= camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		camRotY += camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		camRotY -= camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		camRotZ += camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		camRotZ -= camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		camZoom += camZoomVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		camZoom -= camZoomVel;
	}
}

static void setObjectsPosAndRot()
{
	setObject3Dpos(gridObj, 0, -64, 512);
	setObject3Drot(gridObj, 0, 0, 0);

	setObject3Dpos(cubeObj, 0, 64, 512);
	setObject3Drot(cubeObj, 0, 0, 0);
}

void effectMeshWorldRun()
{
	inputScript();

	setObjectsPosAndRot();

	setCameraPos(camera, 0,0,camZoom);
	setCameraRot(camera, camRotX,camRotY,camRotZ);

	renderObject3D(gridObj, camera);
	renderObject3D(cubeObj, camera);
}
