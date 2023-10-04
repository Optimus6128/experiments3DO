#include "core.h"

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


#define GRID_SIZE 16

static Mesh *gridMesh;
static Mesh *cubeMesh[8];
static Object3D *gridObj;
static Object3D *cubeObj[54];

static Mesh *gridTempleMesh;
static Object3D *gridTempleObj;

static Object3D *elongoidObj;
static Object3D *columnoidObj[27];
static Mesh *columnoidMesh;

static Object3D *columnBigObj[4];
static Object3D *columnSmallObj;
static Object3D *templeRoofObj;
static Object3D *templeBaseObj;
static Mesh *columnBigMesh;
static Mesh *columnSmallMesh;
static Mesh *templeRoofMesh;
static Mesh *templeBaseMesh;


static Object3D *softObj;
static Texture *cloudTex16;

static Mesh *starsMesh;
static Object3D *starsObj;

static Texture *flatTex;
static Texture *flatTex2;
static Texture *flatGridTex[2];
static Texture *panerTex;
static Texture *gridTex;
static Texture *cubeTex;
static uint16 gridPal[32];
static uint16 cubePal[32*8];

static bool autoRot = false;


#define WORLDS_NUM 5

static World *myWorld[WORLDS_NUM];
static int worldIndex = 0;


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

static void prepTempleGrid()
{
	CCB *cel = gridTempleMesh->cel;

	int x,y;
	for (y=0; y<7; ++y) {
		for (x=0; x<7; ++x) {
			int c = (x + y) & 1;
			cel->ccb_SourcePtr = (CelData*)flatGridTex[c]->bitmap;
			++cel;
		}
	}
}

static Object3D *initMeshObject(int meshgenId, const MeshgenParams params, int optionsFlags, Texture *tex)
{
	Object3D *meshObj;

	Mesh *mesh = initGenMesh(meshgenId, params, optionsFlags, tex);
	meshObj = initObject3D(mesh);

	return meshObj;
}

static MeshgenParams initMeshObjectParams(int meshgenId, int sizeScale)
{
	MeshgenParams params;

	switch(meshgenId) {
		case MESH_CUBE:
		{
			params = makeDefaultMeshgenParams(64 * sizeScale);
		}
		break;

		case MESH_SQUARE_COLUMNOID:
		{
			int i;
			const int numPoints = 8;
			const int size = 32 * sizeScale;
			Point2Darray *ptArray = initPoint2Darray(numPoints);

			for (i=0; i<numPoints; ++i) {
				const int y = (size/4) * (numPoints/2 - i);
				const int r = ((SinF16((i*20) << 16) * (size / 2)) >> 16) + size / 2;
				addPoint2D(ptArray, r,y);
			}
			params = makeMeshgenSquareColumnoidParams(size, ptArray->points, numPoints, true, true);

			//destroyPoint2Darray(ptArray); //must destroy it outside after initGenMesh :P
		}
		break;
	}

	return params;
}

static Object3D *initElongoidObject(Texture *tex)
{
	Object3D *meshObj;
	Mesh *mesh;
	MeshgenParams params;
	int i;
	const int numPoints = 128;
	const int size = 64;
	Point2Darray *ptArray = initPoint2Darray(numPoints);

	for (i=0; i<numPoints; ++i) {
		const int y = (size/2) * (numPoints/2 - i);
		int r = ((SinF16((i*20) << 16) * (size / 2)) >> 16) + size / 2;
		r += ((SinF16((i*24) << 15) * (size / 4)) >> 16) + size / 3;
		r += ((SinF16((i*28) << 18) * (size / 8)) >> 16) + size / 4;
		addPoint2D(ptArray, r,y);
	}
	params = makeMeshgenSquareColumnoidParams(size, ptArray->points, numPoints, true, true);

	mesh = initGenMesh(MESH_SQUARE_COLUMNOID, params, MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING, tex);
	meshObj = initObject3D(mesh);

	destroyPoint2Darray(ptArray);

	return meshObj;
}

static MeshgenParams initColumnParams(bool isBig)
{
	MeshgenParams params;

	const int r = 36;
	const int r2 = 28;
	const int scaler = 20;

	if (isBig) {
		Point2Darray *ptArray = initPoint2Darray(5);

		addPoint2D(ptArray, r,16*scaler);
		addPoint2D(ptArray, r2,15*scaler);
		addPoint2D(ptArray, r2,3*scaler);
		addPoint2D(ptArray, r,2*scaler);
		addPoint2D(ptArray, r,0*scaler);

		params = makeMeshgenSquareColumnoidParams(r, ptArray->points, 5, false, false);
	} else {
		Point2Darray *ptArray = initPoint2Darray(5);

		addPoint2D(ptArray, r,4*scaler);
		addPoint2D(ptArray, r2,3*scaler);
		addPoint2D(ptArray, r2,2*scaler);
		addPoint2D(ptArray, r,1*scaler);
		addPoint2D(ptArray, r,0*scaler);

		params = makeMeshgenSquareColumnoidParams(r, ptArray->points, 5, true, false);
	}

	return params;
}

static MeshgenParams initBaseParams()
{
	MeshgenParams params;
	Point2Darray *ptArray = initPoint2Darray(3);

	addPoint2D(ptArray, 228,0);
	addPoint2D(ptArray, 244,-64);
	addPoint2D(ptArray, 228,-128);

	params = makeMeshgenSquareColumnoidParams(456, ptArray->points, 3, false, true);

	return params;
}


static World *initMyWorld(int worldIndex, Camera *camera)
{
	int i;

	World *world = initWorld(128, 1, 1);

	addCameraToWorld(camera, world);

	if (worldIndex != 4) addObjectToWorld(gridObj, 0, false, world);

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
			for (i=0; i<27; ++i) {
				addObjectToWorld(columnoidObj[i], 1, true, world);
			}
		}
		break;

		case 3:
		{
			addObjectToWorld(elongoidObj, 1, false, world);
		}
		break;

		case 4:
		{
			addObjectToWorld(starsObj, 0, false, world);

			addObjectToWorld(gridTempleObj, 1, false, world);
			addObjectToWorld(templeBaseObj, 1, false, world);

			for (i=0; i<4; ++i) {
				addObjectToWorld(columnBigObj[i], 2, true, world);
				setObject3Dpos(columnBigObj[i], 192 * ((i & 1) * 2 - 1), 0, 192 * ((i >> 1) * 2 - 1));
			}

			addObjectToWorld(templeRoofObj, 2, true, world);
			setObject3Dpos(templeRoofObj, 0,320,0);

			addObjectToWorld(softObj, 2, true, world);
			setRenderSoftMethod(RENDER_SOFT_METHOD_GOURAUD);

			addObjectToWorld(columnSmallObj, 2, true, world);
		}
		break;
	}

	return world;
}

void effectMeshWorldInit()
{
	int i,x,y,z;
	static unsigned char paramStretch = 4;
	static uint16 paramCol = 0xFFFF;
	static uint16 paramCol3 = MakeRGB15(16,16,16);
	
	static uint16 paramCol1 = MakeRGB15(15,3,7);
	static uint16 paramCol2 = MakeRGB15(7,3,15);

	MeshgenParams gridParams = makeMeshgenGridParams(2048, GRID_SIZE);
	MeshgenParams cubeParams = DEFAULT_MESHGEN_PARAMS(128);
	MeshgenParams columnoidParams = initMeshObjectParams(MESH_SQUARE_COLUMNOID, 2);
	MeshgenParams columnoidParams2 = initMeshObjectParams(MESH_SQUARE_COLUMNOID, 1);
	MeshgenParams columnBigParams = initColumnParams(true);
	MeshgenParams columnSmallParams = initColumnParams(false);
	MeshgenParams gridTempleParams = makeMeshgenGridParams(456, 7);
	MeshgenParams columnBaseParams = initBaseParams();
	MeshgenParams starsParams = makeMeshgenStarsParams(NUM_REC_Z/4, 256);

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

	flatTex = initGenTexture(4,4, 16, NULL, 0, TEXGEN_FLAT, &paramCol);
	flatTex2 = initGenTexture(4,4, 16, NULL, 0, TEXGEN_FLAT, &paramCol3);
	panerTex = initGenTexture(16,16, 8, cubePal, 1, TEXGEN_XOR, &paramStretch);
	gridTex = initGenTexture(16,16, 8, gridPal, 1, TEXGEN_GRID, NULL);
	cubeTex = initGenTexture(64,64, 8, cubePal, 8, TEXGEN_CLOUDS, NULL);
	
	flatGridTex[0] = initGenTexture(4,4, 16, NULL, 0, TEXGEN_FLAT, &paramCol1);
	flatGridTex[1] = initGenTexture(4,4, 16, NULL, 0, TEXGEN_FLAT, &paramCol2);
	
	starsMesh = initGenMesh(MESH_STARS, starsParams, MESH_OPTION_RENDER_POINTS | MESH_OPTION_NO_TRANSLATE, NULL);
	starsObj = initObject3D(starsMesh);

	gridMesh = initGenMesh(MESH_GRID, gridParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, gridTex);
	gridObj = initObject3D(gridMesh);

	gridTempleMesh = initGenMesh(MESH_GRID, gridTempleParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT, flatGridTex[0]);
	prepTempleGrid();
	gridTempleObj = initObject3D(gridTempleMesh);

	for (i=0; i<8; ++i) {
		cubeMesh[i] = initGenMesh(MESH_CUBE, cubeParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT | MESH_OPTION_ENABLE_LIGHTING, cubeTex);
	}

	for (i=0; i<54; ++i) {
		cubeObj[i] = initObject3D(cubeMesh[i&7]);
		setMeshPaletteIndex(cubeObj[i]->mesh, i & 7);
	}

	columnoidMesh = initGenMesh(MESH_SQUARE_COLUMNOID, columnoidParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT | MESH_OPTION_ENABLE_LIGHTING, panerTex);
	for (i=0; i<27; ++i) {
		columnoidObj[i] = initObject3D(columnoidMesh);
	}
	elongoidObj = initElongoidObject(flatTex);


	columnBigMesh = initGenMesh(MESH_SQUARE_COLUMNOID, columnBigParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING, /*gridTex*/flatTex);
	columnBigMesh = subdivMesh(columnBigMesh);
	columnSmallMesh = initGenMesh(MESH_SQUARE_COLUMNOID, columnSmallParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING, flatTex);
	for (i=0; i<4; ++i) {
		columnBigObj[i] = initObject3D(columnBigMesh);
	}
	columnSmallObj = initObject3D(columnSmallMesh);

	templeRoofMesh = initGenMesh(MESH_PRISM, DEFAULT_MESHGEN_PARAMS(456), MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT | MESH_OPTION_ENABLE_LIGHTING, /*gridTex*/flatTex2);
	templeRoofMesh = subdivMesh(templeRoofMesh);
	templeRoofMesh = subdivMesh(templeRoofMesh);
	templeRoofObj = initObject3D(templeRoofMesh);

	templeBaseMesh = initGenMesh(MESH_SQUARE_COLUMNOID, columnBaseParams, MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYSORT | MESH_OPTION_ENABLE_LIGHTING, /*gridTex*/flatTex);
	templeBaseMesh = subdivMesh(templeBaseMesh);
	templeBaseObj = initObject3D(templeBaseMesh);

	cloudTex16 = initGenTexture(64, 64, 16, NULL, 1, TEXGEN_CLOUDS, NULL);
	softObj = initMeshObject(MESH_SQUARE_COLUMNOID, columnoidParams2, MESH_OPTION_RENDER_SOFT16 | MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_ENABLE_ENVMAP, cloudTex16);

	shadeGrid();

	viewer = createViewer(64,192,64, 176);
	setViewerPos(viewer, 0,96,-1024);

	setGlobalLightDir(-3,-2,1);

	for (i=0; i<WORLDS_NUM; ++i) {
		myWorld[i] = initMyWorld(i, viewer->camera);
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
		break;

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
		break;

		case 2:
		{
			const int dist = 256;
			for (k=-1; k<=1; k++) {
				for (j=-1; j<=1; j++) {
					for (i=-1; i<=1; i++) {
						setObject3Dpos(columnoidObj[n], dist*i, dist + dist*j, dist*k);
						setObject3Drot(columnoidObj[n], i*softRotX, ((j+1) & 3)*softRotY, k*softRotZ);
						++n;
					}
				}
			}
		}
		break;

		case 3:
		{
			setObject3Dpos(elongoidObj, 0, 2048, 0);
			setObject3Drot(elongoidObj, 0, softRotY, 0);
		}
		break;

		case 4:
		{
			setObject3Dpos(softObj, 0, 128 + (SinF16(getTicks() << 14) >> 13), 0);
			setObject3Drot(softObj, 2*softRotX, 2*softRotY, 2*softRotZ);
		}
		break;
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
