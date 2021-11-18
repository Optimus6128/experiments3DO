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

const int rotVel = 3;
const int zoomVel = 32;

static Mesh *testMesh;
static Texture *xorTex;


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
	xorTex = initGenTexture(32,32,8,NULL,1,TEXGEN_XOR, false, NULL);
	testMesh = initGenMesh(1024, xorTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
}

void effectMeshSoftRun()
{
	inputScript();

	setMeshPosition(testMesh, 0, 0, zoom);
	setMeshRotation(testMesh, rotX, rotY, rotZ);

	renderMeshSoft(testMesh);
}
