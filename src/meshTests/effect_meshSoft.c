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

static const int rotVel = 3;
static const int zoomVel = 32;

static Mesh *cubeMesh;


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
}

void effectMeshSoftInit()
{
	cubeMesh = initGenMesh(MESH_CUBE, DEFAULT_MESHGEN_PARAMS(1024), MESH_OPTION_RENDER_SOFT, NULL);
	cubeObj = initObject3D(cubeMesh);
}

void effectMeshSoftRun()
{
	inputScript();

	setObject3Dpos(cubeObj, 0, 0, zoom);
	setObject3Drot(cubeObj, rotX, rotY, rotZ);

	renderObject3Dsoft(cubeObj);
}
