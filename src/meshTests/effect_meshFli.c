#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"
#include "anim_fli.h"

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

static Mesh *gridMesh;
static Object3D *gridObj;
static Texture *gridTex;

static Object3D *screenObj[3];
static Mesh *screenMesh[3];
static Texture *screenTex[3];

static uint16 gridPal[32];

static World *myWorld;

AnimFLI *fli[3];


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

void effectMeshFliInit()
{
	int i;

	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);

	setPalGradient(0,31, 1,3,7, 31,27,23, gridPal);
	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, NULL);
	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, gridTex);

	gridObj = initObject3D(gridMesh);
	shadeGrid();

	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,96,-1024);

	light = createLight(true);

	myWorld = initWorld(128, 1, 1);

	addObjectToWorld(gridObj, 0, false, myWorld);

	addCameraToWorld(viewer->camera, myWorld);
	addLightToWorld(light, myWorld);


	for (i=0; i<3; ++i) {
		screenTex[i] = initTexture(320, 200, 16, TEXTURE_TYPE_DEFAULT, NULL, NULL, 0);
		screenMesh[i] = initGenMesh(MESH_PLANE, makeDefaultMeshgenParams(256), MESH_OPTION_NO_POLYCLIP, screenTex[i]);
		setMeshPolygonOrder(screenMesh[i], true, true);
		screenObj[i] = initObject3D(screenMesh[i]);
		addObjectToWorld(screenObj[i], 1, true, myWorld);
	}

	/*fli[0] = newAnimFLI("data/fly.fli", (uint16*)screenTex[0]->bitmap);
	FLIload(fli[0], true);
	fli[1] = newAnimFLI("data/toyman01.fli", (uint16*)screenTex[1]->bitmap);
	FLIload(fli[1], true);
	fli[2] = newAnimFLI("data/apple.fli", (uint16*)screenTex[2]->bitmap);
	FLIload(fli[2], true);*/

	/*fli[0] = newAnimFLI("data/robotrk.fli", (uint16*)screenTex[0]->bitmap);
	FLIload(fli[0], true);
	fli[1] = newAnimFLI("data/toyman01.fli", (uint16*)screenTex[1]->bitmap);
	FLIload(fli[1], false);
	fli[2] = newAnimFLI("data/apple.fli", (uint16*)screenTex[2]->bitmap);
	FLIload(fli[2], true);*/

	fli[0] = newAnimFLI("data/robotrk.fli", (uint16*)screenTex[0]->bitmap);
	FLIload(fli[0], false);
	fli[1] = newAnimFLI("data/colin.fli", (uint16*)screenTex[1]->bitmap);
	FLIload(fli[1], false);
	fli[2] = newAnimFLI("data/catnap.fli", (uint16*)screenTex[2]->bitmap);
	FLIload(fli[2], false);
}

static void inputScript(int dt)
{
	viewerInputFPS(viewer, dt);
}

static void setObjectsPosAndRot(int dt)
{
	setObject3Dpos(gridObj, 0, 0, 0);
	setObject3Drot(gridObj, 0, 0, 0);

	setObject3Dpos(screenObj[0], 0, 256, 0);
	setObject3Drot(screenObj[0], 0, 0, 0);

	setObject3Dpos(screenObj[1], -256, 256, -128);
	setObject3Drot(screenObj[1], 0, 32, 0);

	setObject3Dpos(screenObj[2], 256, 256, -128);
	setObject3Drot(screenObj[2], 0, -32, 0);
}

void effectMeshFliRun()
{
	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	setObjectsPosAndRot(dt);

	FLIplayNextFrame(fli[0]);
	FLIplayNextFrame(fli[1]);
	FLIplayNextFrame(fli[2]);

	renderWorld(myWorld);
}
