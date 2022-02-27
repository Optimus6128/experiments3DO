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
	cubeMesh8 = initGenMesh(MESH_CUBE_TRI, DEFAULT_MESHGEN_PARAMS(1024), MESH_OPTION_RENDER_SOFT8 | MESH_OPTION_ENABLE_LIGHTING, NULL);
	cubeMesh16 = initGenMesh(MESH_CUBE_TRI, DEFAULT_MESHGEN_PARAMS(1024), MESH_OPTION_RENDER_SOFT16 | MESH_OPTION_ENABLE_LIGHTING, NULL);
	cubeMeshSemiSoft = initGenMesh(MESH_CUBE_TRI, DEFAULT_MESHGEN_PARAMS(1024), MESH_OPTION_RENDER_SEMISOFT | MESH_OPTION_ENABLE_LIGHTING, NULL);

	cubeObj = initObject3D(cubeMesh8);
}

void effectMeshSoftRun()
{
	inputScript();

	setObject3Dpos(cubeObj, 0, 0, zoom);
	setObject3Drot(cubeObj, rotX, rotY, rotZ);
	
	renderObject3Dsoft(cubeObj);
}
