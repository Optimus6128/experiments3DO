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


#define HEIGHTMAP_WIDTH 32
#define HEIGHTMAP_HEIGHT 32
#define HEIGHTMAP_SIZE (HEIGHTMAP_WIDTH * HEIGHTMAP_HEIGHT)
#define HEIGHTMAP_GRADIENTS 32

#define VOXEL_WIDTH 16
#define VOXEL_HEIGHT 16
#define VOXEL_SIZE (VOXEL_WIDTH * VOXEL_HEIGHT)

static Viewer *viewer;
static Light *light;

static Texture *voxelTex;

static Mesh *heightmapMesh;
static Object3D *heightmapObj;
static Texture *heightmapTex;

static World *myWorld;



#define GRID_SIZE 16
static Mesh *gridMesh;
static Object3D *gridObj;
static Texture *gridTex;
static uint16 gridPal[32];


static void initHeightmap()
{
	const int scaleMul = 12;
	const int heightMul = 4;

	int i,j;
	unsigned char *src;
	Vertex *v = heightmapMesh->vertex;
	PolyData *p = heightmapMesh->poly;

	heightmapTex = initGenTexture(HEIGHTMAP_WIDTH,HEIGHTMAP_HEIGHT, 8,NULL, 1, TEXGEN_CLOUDS, NULL);

	for (i=0; i<HEIGHTMAP_GRADIENTS; ++i) {
		uint16 *dst = (uint16*)voxelTex[i].bitmap;
		const uint16 c = MakeRGB15(i,i,i);
		for (j=0; j<VOXEL_SIZE; ++j) {
			*dst++ = c;
		}
	}

	src = heightmapTex->bitmap;
	for (j=0; j<HEIGHTMAP_HEIGHT; ++j) {
		const int yc = j - HEIGHTMAP_HEIGHT / 2;
		for (i=0; i<HEIGHTMAP_WIDTH; ++i) {
			const int xc = i - HEIGHTMAP_WIDTH / 2;
			int height = *src++;
			
			CLAMP(height, 0, HEIGHTMAP_GRADIENTS-1)

			v->x = xc * scaleMul;
			v->y = 64 + height * heightMul;
			v->z = yc * scaleMul;
			++v;

			p->textureId = height;
			++p;
		}
	}
}

void effectMeshHeightmapInit()
{
	MeshgenParams heightmapParams = makeMeshgenParticlesParams(HEIGHTMAP_SIZE);

	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);
	setPalGradient(0,31, 1,3,7, 31,27,23, gridPal);
	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, NULL);
	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, gridTex);
	gridObj = initObject3D(gridMesh);

	voxelTex = initTextures(VOXEL_WIDTH,VOXEL_HEIGHT,16,TEXTURE_TYPE_DEFAULT, NULL, NULL, 0, HEIGHTMAP_GRADIENTS);

	heightmapMesh = initGenMesh(MESH_PARTICLES, heightmapParams, MESH_OPTION_RENDER_BILLBOARDS, voxelTex);
	heightmapObj = initObject3D(heightmapMesh);

	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,96,-1024);

	light = createLight(true);

	myWorld = initWorld(128, 1, 1);

	//addObjectToWorld(gridObj, 0, false, myWorld);
	addObjectToWorld(heightmapObj, 1, false, myWorld);

	addCameraToWorld(viewer->camera, myWorld);
	addLightToWorld(light, myWorld);

	initHeightmap();
}

static void inputScript(int dt)
{
	viewerInputFPS(viewer, dt);
}

void effectMeshHeightmapRun()
{
	static int prevTicks = 0;
	int currTicks = getTicks();
	int dt = currTicks - prevTicks;
	prevTicks = currTicks;

	inputScript(dt);

	renderWorld(myWorld);
}
