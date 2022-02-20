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

static Mesh *testMesh;
static Texture *xorTex;
static uint16 texPal[32];


static Object3D *testObj;

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
	xorTex = initGenTexture(32,32,8,texPal,1,TEXGEN_XOR, false, NULL);
	testMesh = initGenMesh(MESH_OPTIONS_DEFAULT, MESH_CUBE, DEFAULT_MESHGEN_PARAMS(1024), xorTex);
	testObj = initObject3D(testMesh);

	setPal(0,31, 48,64,192, 160,64,32, texPal, 3);
}

void effectMeshSoftRun()
{
	inputScript();

	setObject3Dpos(testObj, 0, 0, zoom);
	setObject3Drot(testObj, rotX, rotY, rotZ);

	//renderObject3D(testObj);
	renderObject3Dsoft(testObj);
}
