#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"
#include "file_utils.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"
#include "engine_world.h"
#include "engine_view.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"


#define GRID_SIZE 16

static Viewer *viewer;
static Light *light;

static Object3D *loadedObj;
static Mesh *loadedMesh;

static Mesh *gridMesh;
static Object3D *gridObj;

static Texture *flatTex;
static Texture *gridTex;

static uint16 gridPal[32];

static bool autoRot = false;

static World *myWorld;


static void shadeGrid()
{
	int x,y;
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

void effectMeshLoadInit()
{
	static unsigned short paramCol = 0x7fff;
	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);
	
	setPalGradient(0,31, 1,3,7, 31,27,23, gridPal);

	flatTex = initGenTexture(4,4, 16, NULL, 0, TEXGEN_FLAT, &paramCol);
	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, NULL);

	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, gridTex);
	
	loadedMesh = loadMesh("data/head.3do", MESH_LOAD_SKIP_LINES | MESH_LOAD_FLIP_POLYORDER, MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING, flatTex);
	loadedObj = initObject3D(loadedMesh);


	gridObj = initObject3D(gridMesh);
	shadeGrid();

	viewer = createViewer(64,192,64, 176);
	//setViewerPos(viewer, 0,96,-1024);
	setViewerPos(viewer, 0,160,-320);

	light = createLight(true);

	myWorld = initWorld(128, 1, 1);
	
	addObjectToWorld(gridObj, 0, false, myWorld);

	addCameraToWorld(viewer->camera, myWorld);
	addLightToWorld(light, myWorld);
	addObjectToWorld(loadedObj, 1, true, myWorld);
}

static void inputScript(int dt)
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		autoRot = !autoRot;
	}

	viewerInputFPS(viewer, dt);
}

static void setObjectsPosAndRot(int dt)
{
	static int softRotX = 0;
	static int softRotY = 128;
	static int softRotZ = 0;

	setObject3Dpos(gridObj, 0, 0, 0);
	setObject3Drot(gridObj, 0, 0, 0);

	setObject3Dpos(loadedObj, 0, 256, 0);
	setObject3Drot(loadedObj, softRotX, softRotY, softRotZ);

	/*drawNumber(32,40, loadedMesh->verticesNum);
	drawNumber(32,48, loadedMesh->polysNum);
	drawNumber(32,56, loadedMesh->indicesNum);
	drawNumber(32,64, loadedMesh->linesNum);*/

	if (autoRot) {
		//softRotX += 1;
		softRotY += 2;
		//softRotZ -= 1;
	}
}

void effectMeshLoadRun()
{
	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	setObjectsPosAndRot(dt);

	renderWorld(myWorld);
}
