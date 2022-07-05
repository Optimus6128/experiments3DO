#include "core.h"

#include "effect_meshWorld.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"
#include "engine_world.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"


static Camera *camera;
static Light *light;
static int camHeight = 128;

#define GRID_SIZE 16

static Mesh *gridMesh;
static Mesh *cubeMesh;
static Object3D *gridObj;
static Object3D *cubeObj[8];

static Object3D *softObj;
static Texture *cloudTex16;

static Texture *gridTex;
static Texture *cubeTex;
static uint16 gridPal[32];
static uint16 cubePal[32];

static int camRotX = 0;
static int camRotY = 0;
static int camRotZ = 0;
static int camPosX = 0;
static int camPosY = 0;
static int camPosZ = -768 << FP_CORE;

static int camRotVel = 2;
static int camMoveVel = 12;
static int camFlyVel = 8;

static bool autoRot = false;

static World *world;

static int renderSoftMethodIndex = RENDER_SOFT_METHOD_GOURAUD;


static void shadeGrid()
{
	int x,y;
	//Vertex *dstVertex = gridMesh->vertex;
	CCB *cel = gridMesh->cel;

	for (y=0; y<GRID_SIZE; ++y) {
		const int yc = y - GRID_SIZE / 2;
		for (x=0; x<GRID_SIZE; ++x) {
			const int xc = x - GRID_SIZE / 2;
			int r = (isqrt(xc*xc + yc*yc) * 16) / (GRID_SIZE / 2);
			CLAMP(r,0,15)
			cel->ccb_PIXC = shadeTable[15-r];
			++cel;
		}
	}
}

static Object3D *initMeshObjectSoft(int meshgenId, const MeshgenParams params, int optionsFlags, Texture *tex)
{
	Object3D *meshObj;

	Mesh *softMesh = initGenMesh(meshgenId, params, optionsFlags, tex);
	meshObj = initObject3D(softMesh);
	setObject3Dmesh(meshObj, softMesh);

	return meshObj;
}

static MeshgenParams initMeshObjectParams(int meshgenId)
{
	MeshgenParams params;

	switch(meshgenId) {
		case MESH_CUBE:
		{
			params = makeDefaultMeshgenParams(64);
		}
		break;

		case MESH_SQUARE_COLUMNOID:
		{
			int i;
			const int numPoints = 8;
			const int size = 40;
			Point2Darray *ptArray = initPoint2Darray(numPoints);

			for (i=0; i<numPoints; ++i) {
				const int y = (size/4) * (numPoints/2 - i);
				const int r = ((SinF16((i*20) << 16) * (size / 2)) >> 16) + size / 2;
				addPoint2D(ptArray, r,y);
			}
			params = makeMeshgenSquareColumnoidParams(size, ptArray->points, numPoints, true, true);

			//destroyPoint2Darray(ptArray); //why it crashes now?
		}
		break;
	}

	return params;
}

static void initMyWorld()
{
	int i;

	world = initWorld(16, 1, 1);

	camera = createCamera();
	light = createLight(true);

	addCameraToWorld(camera, world);
	addLightToWorld(light, world);

	addObjectToWorld(gridObj, 0, world);

	for (i=0; i<8; ++i) {
		addObjectToWorld(cubeObj[i], 1, world);
	}
	addObjectToWorld(softObj, 1, world);
}

void effectMeshWorldInit()
{
	int i;

	MeshgenParams gridParams = makeMeshgenGridParams(1024, GRID_SIZE);
	MeshgenParams cubeParams = DEFAULT_MESHGEN_PARAMS(128);

	//MeshgenParams softParams = initMeshObjectParams(MESH_CUBE);
	MeshgenParams softParams = initMeshObjectParams(MESH_SQUARE_COLUMNOID);

	setPalGradient(0,31, 1,3,7, 27,29,31, gridPal);
	setPalGradient(0,31, 15,3,7, 7,31,15, cubePal);

	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, false, NULL);
	cubeTex = initGenTexture(64,64, 8, cubePal, 1, TEXGEN_CLOUDS, false, NULL);

	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT, gridTex);
	gridObj = initObject3D(gridMesh);

	cubeMesh = initGenMesh(MESH_CUBE, cubeParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING, cubeTex);

	for (i=0; i<8; ++i) {
		cubeObj[i] = initObject3D(cubeMesh);
	}

	cloudTex16 = initGenTexture(64, 64, 16, NULL, 1, TEXGEN_CLOUDS, false, NULL);
	//softObj = initMeshObjectSoft(MESH_CUBE, softParams, MESH_OPTION_RENDER_SOFT16 | MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_ENABLE_ENVMAP, cloudTex16);
	softObj = initMeshObjectSoft(MESH_SQUARE_COLUMNOID, softParams, MESH_OPTION_RENDER_SOFT16 | MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_ENABLE_ENVMAP, cloudTex16);

	shadeGrid();

	initMyWorld();
}

static bool tryCollideMoveStep(bool x, bool y, vec3f16 move, int speed)
{
	const int off = 192 << FP_CORE;

	int prevCamPosX = camPosX;
	int prevCamPosZ = camPosZ;

	if (x) camPosX += move[0] * speed;
	if (y) camPosZ += move[2] * speed;

	if (camPosX>-off && camPosX<off && camPosZ>-off && camPosZ<off) {
		camPosX = prevCamPosX;
		camPosZ = prevCamPosZ;
		return true;
	}
	return false;
}

static void moveCamera(int forward, int right, int up, int dt)
{
	static mat33f16 rotMat;
	static vec3f16 move;

	move[0] = (right << FP_BASE) >> 1;
	move[1] = up << FP_BASE;
	move[2] = forward << FP_BASE;

	createRotationMatrixValues(camRotX, camRotY, camRotZ, (int*)rotMat);	// not correct when looking up/down yet
	MulVec3Mat33_F16(move, move, rotMat);

	if (tryCollideMoveStep(true, true, move, camMoveVel * dt)) {
		if(tryCollideMoveStep(true, false, move, camMoveVel * dt)) {
			tryCollideMoveStep(false, true, move, camMoveVel * dt);
		}
	}

	camPosY += move[1] * camFlyVel * dt;
	if (camPosY <0) camPosY = 0;
}

static void inputScript(int dt)
{
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		camRotY += camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		camRotY -= camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		moveCamera(1,0,0,dt);
	}

	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		moveCamera(-1,0,0,dt);
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		moveCamera(0,0,1,dt);
		//camRotX += camRotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		moveCamera(0,0,-1,dt);
		//camRotX -= camRotVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		if (++renderSoftMethodIndex == RENDER_SOFT_METHOD_NUM) renderSoftMethodIndex = 0;
		setRenderSoftMethod(renderSoftMethodIndex);
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		autoRot = !autoRot;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		moveCamera(0,-1,0,dt);
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		moveCamera(0,1,0,dt);
	}
}

static void setObjectsPosAndRot(int dt)
{
	int i,j,k,n=0;

	static int softRotX = 0;
	static int softRotY = 0;
	static int softRotZ = 0;

	setObject3Dpos(gridObj, 0, 0, 0);
	setObject3Drot(gridObj, 0, 0, 0);

	for (k=-1; k<=1; k+=2) {
		for (j=-1; j<=1; j+=2) {
			for (i=-1; i<=1; i+=2) {
				setObject3Dpos(cubeObj[n], 128*i, 192 + 128*j, 128*k);
				setObject3Drot(cubeObj[n], i*softRotX, j*softRotY, k*softRotZ);
				++n;
			}
		}
	}

	setObject3Dpos(softObj, 0, 192 + (SinF16(getTicks() << 14) >> 13), 0);
	setObject3Drot(softObj, softRotX, softRotY, softRotZ);

	if (autoRot) {
		softRotX += 1;
		softRotY += 2;
		softRotZ -= 1;
	}
}

void effectMeshWorldRun()
{
	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	setObjectsPosAndRot(dt);

	setCameraPos(camera, camPosX>>FP_CORE, camHeight + (camPosY>>FP_CORE), camPosZ>>FP_CORE);
	setCameraRot(camera, camRotX,camRotY,camRotZ);

	renderWorld(world);
}
