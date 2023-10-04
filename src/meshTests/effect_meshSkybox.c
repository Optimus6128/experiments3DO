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


#define SKYBOX_SIDES 6


static Viewer *viewer;
static Light *light;

static Object3D *skyboxObj;
static Object3D *loadedObj;
static Mesh *loadedMesh;

static Texture *flatTex;

static char* skyboxTexSideName[SKYBOX_SIDES] = { "front", "right", "back", "left", "top", "bottom" };
static Texture skyboxTex[SKYBOX_SIDES];

static bool autoRot = false;

static World *myWorld;


static void initSkyboxTextures()
{
	int i;
	char *filepath = (char*)AllocMem(64, MEMTYPE_ANY);
	Texture *tempTex;

	for (i=0; i<SKYBOX_SIDES; ++i) {
		sprintf(filepath, "data/skybox/256/%s.cel", skyboxTexSideName[i]);
		tempTex = loadTexture(filepath);
		memcpy(&skyboxTex[i], tempTex, sizeof(Texture));
		FreeMem(tempTex, sizeof(Texture));
	}

	FreeMem(filepath, 64);
}

static void initSkyboxObject()
{
	MeshgenParams params = makeMeshgenSkyboxParams(4096, 2);
	Mesh *skyboxMesh;

	initSkyboxTextures();

	skyboxMesh = initGenMesh(MESH_SKYBOX, params, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT | MESH_OPTION_NO_TRANSLATE, skyboxTex);

	skyboxObj = initObject3D(skyboxMesh);

	addObjectToWorld(skyboxObj, 0, true, myWorld);
}

static void initTeapotObject()
{
	static unsigned char paramCol = 0x7f;

	flatTex = initGenTexture(4,4, 16, NULL, 0, TEXGEN_FLAT, &paramCol);

	loadedMesh = loadMesh("data/duck2.3do", MESH_LOAD_SKIP_LINES /*| MESH_LOAD_FLIP_POLYORDER*/, MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING, flatTex);
	scaleMesh(loadedMesh, 4, 4, 4);
	loadedObj = initObject3D(loadedMesh);

	addObjectToWorld(loadedObj, 1, true, myWorld);
}

void effectMeshSkyboxInit()
{
	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,96,-1024);

	light = createLight(true);

	myWorld = initWorld(128, 1, 1);

	addCameraToWorld(viewer->camera, myWorld);
	addLightToWorld(light, myWorld);

	initSkyboxObject();
	initTeapotObject();
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
	static int softRotX = -64;
	static int softRotY = 0;
	static int softRotZ = 0;

	setObject3Dpos(loadedObj, 0, 256, 0);
	setObject3Drot(loadedObj, softRotX, softRotY, softRotZ);

	/*drawNumber(32,40, loadedMesh->verticesNum);
	drawNumber(32,48, loadedMesh->polysNum);
	drawNumber(32,56, loadedMesh->indicesNum);
	drawNumber(32,64, loadedMesh->linesNum);*/

	if (autoRot) {
		//softRotX += 1;
		//softRotY += 2;
		softRotZ += 1;
	}
}

void effectMeshSkyboxRun()
{
	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	setObjectsPosAndRot(dt);

	renderWorld(myWorld);

	//displayDebugNums(false);
}
