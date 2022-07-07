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
#include "engine_view.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"


static Viewer *viewer;
static Light *light;


#define GRID_SIZE 64

static Mesh *gridMesh;
static Mesh *cubeMesh[8];
static Object3D *gridObj;
static Object3D *cubeObj[54];

static Object3D *softObj;
static Texture *cloudTex16;

static Texture *gridTex;
static Texture *cubeTex;
static uint16 gridPal[32];
static uint16 cubePal[32*8];

static bool autoRot = false;


#define WORLDS_NUM 4

static World *myWorld[WORLDS_NUM];
static int worldIndex = 0;


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
			const int size = 64;
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

static World *initMyWorld(int worldIndex, Camera *camera, Light *light)
{
	int i;

	World *world = initWorld(256, 1, 1);

	addCameraToWorld(camera, world);
	addLightToWorld(light, world);

	addObjectToWorld(gridObj, 0, false, world);

	switch(worldIndex) {
		case 0:
		{
			for (i=0; i<8; ++i) {
				addObjectToWorld(cubeObj[i], 1, true, world);
			}
			addObjectToWorld(softObj, 1, true, world);
			setRenderSoftMethod(RENDER_SOFT_METHOD_ENVMAP);
		}
		break;

		case 1:
		{
			for (i=0; i<54; ++i) {
				addObjectToWorld(cubeObj[i], 1, true, world);
			}
		}
		break;

		case 2:
		{
		}
		break;

		case 3:
		{

		}
		break;
	}

	return world;
}

void effectMeshWorldInit()
{
	int i,x,y,z;

	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);
	MeshgenParams cubeParams = DEFAULT_MESHGEN_PARAMS(128);
	MeshgenParams softParams = initMeshObjectParams(MESH_SQUARE_COLUMNOID);

	setPalGradient(0,31, 1,3,7, 31,27,23, gridPal);

	i = 0;
	for (z=0; z<=1; ++z) {
		for (y=0; y<=1; ++y) {
			for (x=0; x<=1; ++x) {
				setPalGradient(0,31, (1-x)*15,(1-y)*7,(1-z)*3, x*24,y*27,z*31, &cubePal[32*i]);
				++i;
			}
		}
	}

	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, false, NULL);
	cubeTex = initGenTexture(64,64, 8, cubePal, 8, TEXGEN_CLOUDS, false, NULL);

	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT, gridTex);
	gridObj = initObject3D(gridMesh);

	for (i=0; i<8; ++i) {
		cubeMesh[i] = initGenMesh(MESH_CUBE, cubeParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING, cubeTex);
	}

	for (i=0; i<54; ++i) {
		cubeObj[i] = initObject3D(cubeMesh[i&7]);
		setMeshPaletteIndex(i & 7, cubeObj[i]->mesh);
	}

	cloudTex16 = initGenTexture(64, 64, 16, NULL, 1, TEXGEN_CLOUDS, false, NULL);
	softObj = initMeshObjectSoft(MESH_SQUARE_COLUMNOID, softParams, MESH_OPTION_RENDER_SOFT16 | MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_ENABLE_ENVMAP, cloudTex16);

	shadeGrid();

	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,96,-1024);

	light = createLight(true);

	for (i=0; i<WORLDS_NUM; ++i) {
		myWorld[i] = initMyWorld(i, viewer->camera, light);
	}
}

static void inputScript(int dt)
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_SELECT)) {
		worldIndex++;
		if (worldIndex==WORLDS_NUM) worldIndex = 0;
	}
	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		autoRot = !autoRot;
	}

	viewerInputFPS(viewer, dt);
}

static void setObjectsPosAndRot(int worldI, int dt)
{
	int i,j,k,n=0;

	static int softRotX = 0;
	static int softRotY = 0;
	static int softRotZ = 0;

	setObject3Dpos(gridObj, 0, 0, 0);
	setObject3Drot(gridObj, 0, 0, 0);

	switch(worldI) {
		case 0:
		{
			const int dist = 128;
			for (k=-1; k<=1; k+=2) {
				for (j=0; j<=2; j+=2) {
					for (i=-1; i<=1; i+=2) {
						setObject3Dpos(cubeObj[n], dist*i, dist + dist*j, dist*k);
						setObject3Drot(cubeObj[n], i*softRotX, ((j+1) & 3)*softRotY, k*softRotZ);
						++n;
					}
				}
			}

			setObject3Dpos(softObj, 0, 2 * dist + (SinF16(getTicks() << 14) >> 13), 0);
			setObject3Drot(softObj, softRotX, softRotY, softRotZ);
		}

		case 1:
		{
			const int dist = 256;
			for (k=-1; k<=1; k+=1) {
				for (j=0; j<6; j++) {
					for (i=-1; i<=1; i+=1) {
						setObject3Dpos(cubeObj[n], dist*i, dist + dist*j, dist*k);
						setObject3Drot(cubeObj[n], i*softRotX, ((j+1) & 3)*softRotY, k*softRotZ);
						++n;
					}
				}
			}
		}

		case 2:
		{
		}

		case 3:
		{
		}
	}

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

	setObjectsPosAndRot(worldIndex, dt);

	renderWorld(myWorld[worldIndex]);

	drawNumber(288, 8, worldIndex);
}
