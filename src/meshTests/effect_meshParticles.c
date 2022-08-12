#include "core.h"

#include "effect_meshParticles.h"

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
#define NUM_PARTICLES 256

static Viewer *viewer;
static Light *light;

static Mesh *gridMesh;
static Object3D *gridObj;

static Texture *gridTex;
static uint16 gridPal[32];

static Mesh *particlesMesh;
static Object3D *particlesObj;

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

void effectMeshParticlesInit()
{
	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);
	MeshgenParams particlesParams = makeMeshgenParticlesParams(NUM_PARTICLES);
	
	setPalGradient(0,31, 1,3,7, 31,27,23, gridPal);
	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, NULL);
	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, gridTex);

	gridObj = initObject3D(gridMesh);
	shadeGrid();

	particlesMesh = initGenMesh(MESH_PARTICLES, particlesParams, MESH_OPTION_RENDER_POINTS, NULL);
	particlesObj = initObject3D(particlesMesh);

	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,96,-1024);

	light = createLight(true);

	myWorld = initWorld(128, 1, 1);
	
	addObjectToWorld(gridObj, 0, false, myWorld);
	addObjectToWorld(particlesObj, 1, false, myWorld);

	addCameraToWorld(viewer->camera, myWorld);
	addLightToWorld(light, myWorld);
}

static void inputScript(int dt)
{
	viewerInputFPS(viewer, dt);
}

void effectMeshParticlesRun()
{
	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	renderWorld(myWorld);
}
