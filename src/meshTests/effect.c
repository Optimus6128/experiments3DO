#include "core.h"

#include "effect.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"
#include "procgen_texture.h"


static Mesh *pyramidMesh1, *pyramidMesh2;
static uint16 *pyramidPal;

static Texture *xorTex1, *xorTex2;

static int rotX=0, rotY=0, rotZ=0;
static int zoom=2048;

const int rotVel = 1;
const int zoomVel = 16;

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

void effectInit()
{
	pyramidPal = (uint16*)AllocMem(32 * sizeof(uint16), MEMTYPE_ANY);
	setPal(0,31, 48,64,192, 160,64,32, pyramidPal, 3);

	xorTex1 = initGenTexture(128, 128, 8, pyramidPal, 1, TEXGEN_XOR, NULL);
	xorTex2 = initGenTexture(128, 128, 8, pyramidPal, 1, TEXGEN_XOR, NULL);

	eraseHalfTextureTriangleArea(xorTex2, TRI_AREA_LR_TOP);

	pyramidMesh1 = initGenMesh(1024, xorTex1, MESH_OPTIONS_DEFAULT, MESH_PYRAMID1, NULL);
	pyramidMesh2 = initGenMesh(1024, xorTex2, MESH_OPTIONS_DEFAULT, MESH_PYRAMID2, NULL);
}

void effectRun()
{
	Mesh *mesh = pyramidMesh1;

	inputScript();
	
	setMeshPosition(mesh, 0, 0, zoom);
	setMeshRotation(mesh, rotX, rotY, rotZ);

	transformGeometry(mesh);
	renderTransformedGeometry(mesh);
}
