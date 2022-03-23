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


static int rotX=0, rotY=0, rotZ=0;
static int zoom=256;

static const int rotVel = 2;
static const int zoomVel = 2;

static Mesh *softMesh8;
static Mesh *softMesh16;
static Mesh *softMeshSemi;

static Texture *cloudTex;

static int selectedSoftMesh = 0;
static int renderSoftMethodIndex = RENDER_SOFT_METHOD_GOURAUD;


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

	cloudTex = initGenTexture(128, 128, 16, NULL, 1, TEXGEN_CLOUDS, false, NULL);
	params = makeMeshgenSquareColumnoidParams(size, ptArray->points, numPoints, true, true);

	softMesh8 = initGenMesh(meshType, params, MESH_OPTION_RENDER_SOFT8 | MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_ENABLE_ENVMAP, cloudTex);
	softMesh16 = initGenMesh(meshType, params, MESH_OPTION_RENDER_SOFT16 | MESH_OPTION_ENABLE_LIGHTING | MESH_OPTION_ENABLE_ENVMAP, cloudTex);
	softMeshSemi = initGenMesh(meshType, params, MESH_OPTION_RENDER_SEMISOFT | MESH_OPTION_ENABLE_LIGHTING, NULL);

	softObj = initObject3D(softMesh8);

	destroyPoint2Darray(ptArray);
}

void effectMeshSoftRun()
{
	inputScript();

	setObject3Dpos(softObj, 0, 0, zoom);
	setObject3Drot(softObj, rotX, rotY, rotZ);
	renderObject3Dsoft(softObj);
}
