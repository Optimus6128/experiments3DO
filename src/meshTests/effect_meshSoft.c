#include "core.h"

#include "effect_meshSoft.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_soft.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"

#include "sprite_engine.h"

//0 0 0			39 18 11	20 15 7		131
//-64 0 0		25 14 9		17 13 6		41
//-64 32 0		23 13 8		16 12 6		35


static int rotX=0, rotY=0, rotZ=0;
static int zoom=640;

static const int rotVel = 2;
static const int zoomVel = 2;

static Mesh *softMesh8;
static Mesh *softMesh16;
static Mesh *softMeshSemi;

static Texture *cloudTex8;
static Texture *cloudTex16;

static int selectedSoftMesh = 0;
static int renderSoftMethodIndex = RENDER_SOFT_METHOD_GOURAUD;

static Mesh *draculMesh;
static Object3D *draculObj;
static Texture *draculTex;

static Object3D *softObj;


static void inputScript()
{
	if (isJoyButtonPressed(JOY_BUTTON_LEFT)) {
		rotX += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RIGHT)) {
		rotX -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_UP)) {
		rotY += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_DOWN)) {
		rotY -= rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_A)) {
		rotZ += rotVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_B)) {
		rotZ -= rotVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		++selectedSoftMesh;
		if (selectedSoftMesh==3) selectedSoftMesh = 0;

		if (selectedSoftMesh==0) {
			setObject3Dmesh(softObj, softMesh8);
		} else if (selectedSoftMesh==1) {
			setObject3Dmesh(softObj, softMesh16);
		} else {
			setObject3Dmesh(softObj, softMeshSemi);
		}
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		zoom += zoomVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		zoom -= zoomVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		++renderSoftMethodIndex;
		if (renderSoftMethodIndex == RENDER_SOFT_METHOD_NUM) renderSoftMethodIndex = 0;

		setRenderSoftMethod(renderSoftMethodIndex);
	}
}

void effectMeshSoftInit()
{
	int i;
	const int numPoints = 8;
	const int size = 64;
	MeshgenParams params;

	Point2Darray *ptArray = initPoint2Darray(numPoints);
	
	int meshType = MESH_SQUARE_COLUMNOID;

	for (i=0; i<numPoints; ++i) {
		const int y = (size/4) * (numPoints/2 - i);
		const int r = (int)(sin((float)i / 2.0f) * (size / 2) + size / 2);
		addPoint2D(ptArray, r,y);
	}

	cloudTex8 = initGenTexture(128, 128, 8, NULL, 1, TEXGEN_CLOUDS, false, NULL);
	cloudTex16 = initGenTexture(128, 128, 16, NULL, 0, TEXGEN_CLOUDS, false, NULL);
	params = makeMeshgenSquareColumnoidParams(size, ptArray->points, numPoints, true, true);

	softMesh8 = initGenMesh(meshType, params, MESH_OPTION_RENDER_SOFT8 | MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_ENABLE_ENVMAP, cloudTex8);
	softMesh16 = initGenMesh(meshType, params, MESH_OPTION_RENDER_SOFT16 | MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_ENABLE_ENVMAP, cloudTex16);
	softMeshSemi = initGenMesh(meshType, params, MESH_OPTION_RENDER_SEMISOFT | MESH_OPTION_ENABLE_LIGHTING, NULL);

	softObj = initObject3D(softMesh8);

	destroyPoint2Darray(ptArray);


	setBackgroundColor(0x01020102);

	draculTex = loadTexture("data/draculin64.cel");
	draculMesh = initGenMesh(MESH_CUBE, DEFAULT_MESHGEN_PARAMS(192), MESH_OPTIONS_DEFAULT | MESH_OPTION_ENABLE_LIGHTING, draculTex);
	draculObj = initObject3D(draculMesh);
}

static void renderHardObj(int posX, int posZ, int t)
{
	setObject3Dpos(draculObj, posX, 0, zoom + posZ);
	setObject3Drot(draculObj, t, t<<1, t>>1);
	renderObject3D(draculObj);
}

static void renderSoftObj(int posX, int posZ, int t)
{
	setObject3Dpos(softObj, -posX, 0, zoom + -posZ);
	setObject3Drot(softObj, t<<1, t>>1, t);
	renderObject3Dsoft(softObj);
}

void effectMeshSoftRun()
{
	const int t = getTicks() >> 5;
	int posX = SinF16(t<<16) >> 8;
	int posZ = CosF16(t<<16) >> 8;

	if (posZ < 0) {
		renderSoftObj(posX, posZ, t);
		renderHardObj(posX, posZ, t);
	} else {
		renderHardObj(posX, posZ, t);
		renderSoftObj(posX, posZ, t);
	}

	inputScript();

	//displayDebugNums(true);
}
