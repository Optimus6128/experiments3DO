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
static int camPosX = 0;
static int camPosY = 0;
static int camPosZ = 0;

static int camRotVel = 1;
static int camMoveVel = 4;

void effectMeshWorldInit()
{
	MeshgenParams gridParams = makeMeshgenGridParams(1024, 16);
	MeshgenParams cubeParams = DEFAULT_MESHGEN_PARAMS(256);

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

static void moveCamera(int forward, int right)
{
	static mat33f16 rotMat;
	static vec3f16 move;

	move[0] = right << FP_BASE;
	move[1] = 0;
	move[2] = forward << FP_BASE;

	createRotationMatrixValues(camRotX, camRotY, camRotZ, (int*)rotMat);	// not correct when looking up/down yet
	MulVec3Mat33_F16(move, move, rotMat);

	camPosX += move[0] * camMoveVel;
	camPosY += move[1] * camMoveVel;
	camPosZ += move[2] * camMoveVel;
}

static void inputScript()
{
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		camRotY += camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		camRotY -= camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		moveCamera(1,0);
	}

	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		moveCamera(-1,0);
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		camRotX += camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		camRotX -= camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		moveCamera(0,-1);
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		moveCamera(0,1);
	}
}

static void setObjectsPosAndRot()
{
	setObject3Dpos(gridObj, 0, -64, 1024);
	setObject3Drot(gridObj, 0, 0, 0);

	setObject3Dpos(cubeObj, 0, 64, 1024);
	setObject3Drot(cubeObj, 0, 0, 0);
}

void effectMeshWorldRun()
{
	inputScript();

	setObjectsPosAndRot();

	setCameraPos(camera, camPosX>>FP_BASE, 256 + (camPosY>>FP_BASE), camPosZ>>FP_BASE);
	setCameraRot(camera, camRotX,camRotY,camRotZ);

	renderObject3D(gridObj, camera);
	renderObject3D(cubeObj, camera);
}
