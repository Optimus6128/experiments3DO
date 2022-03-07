#include "core.h"

#include "effect_meshSoft.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"

#include "sprite_engine.h"


static int rotX=0, rotY=0, rotZ=0;
static int zoom=2048;

static const int rotVel = 2;
static const int zoomVel = 32;

static Mesh *cubeMesh8;
static Mesh *cubeMesh16;
static Mesh *cubeMeshSemiSoft;

static int selectedCubeMesh = 0;


static Object3D *cubeObj;

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

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		zoom += zoomVel;
	}

	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		zoom -= zoomVel;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		++selectedCubeMesh;
		if (selectedCubeMesh==3) selectedCubeMesh = 0;

		if (selectedCubeMesh==0) {
			setObject3Dmesh(cubeObj, cubeMesh8);
		} else if (selectedCubeMesh==1) {
			setObject3Dmesh(cubeObj, cubeMesh16);
		} else {
			setObject3Dmesh(cubeObj, cubeMeshSemiSoft);
		}
	}
}

void effectMeshSoftInit()
{
	const int size = 512;
	MeshgenParams params;

	const int r1 = size/4;
	const int r2 = size/2;
	const int r3 = size/4;
	const int y1 = size/2;
	const int y2 = size/4;
	const int y3 = -size/4;
	const int y4 = -size/2;

	Point2Darray *ptArray = initPoint2Darray(4);

	addPoint2D(ptArray, r1,y1);
	addPoint2D(ptArray, r2,y2);
	addPoint2D(ptArray, r2,y3);
	addPoint2D(ptArray, r3,y4);

	params = makeMeshgenSquareColumnoidParams(size, ptArray->points, 4);

	cubeMesh8 = initGenMesh(MESH_SQUARE_COLUMNOID, params, MESH_OPTION_RENDER_SOFT8 | MESH_OPTION_ENABLE_LIGHTING, NULL);
	cubeMesh16 = initGenMesh(MESH_SQUARE_COLUMNOID, params, MESH_OPTION_RENDER_SOFT16 | MESH_OPTION_ENABLE_LIGHTING, NULL);
	cubeMeshSemiSoft = initGenMesh(MESH_SQUARE_COLUMNOID, params, MESH_OPTION_RENDER_SEMISOFT | MESH_OPTION_ENABLE_LIGHTING, NULL);

	cubeObj = initObject3D(cubeMesh8);

	destroyPoint2Darray(ptArray);
}

void effectMeshSoftRun()
{
	inputScript();

	setObject3Dpos(cubeObj, 0, 0, zoom);
	setObject3Drot(cubeObj, rotX, rotY, rotZ);
	
	renderObject3Dsoft(cubeObj);
}
