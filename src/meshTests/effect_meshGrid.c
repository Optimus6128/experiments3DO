#include "core.h"

#include "effect_meshGrid.h"

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


static Mesh *gridMesh;
static Object3D *gridObj;

static Texture *waterTex;
static Texture *cloudTex;
static Sprite *waterSpr;
static uint16 waterPal[32];

static int rotX=0, rotY=0, rotZ=0;
static int zoom=2048;

static const int rotVel = 2;
static const int zoomVel = 8;


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

void effectMeshGridInit()
{
	const int texWidth = 32;
	const int texHeight = 32;
	MeshgenParams params = DEFAULT_MESHGEN_PARAMS(1024);

	setPalGradient(0,31, 1,1,3, 27,29,31, waterPal);
	waterTex = initGenTexture(texWidth, texHeight, 8, waterPal, 1, TEXGEN_EMPTY, false, NULL);
	cloudTex = initGenTexture(texWidth, texHeight, 8, waterPal, 1, TEXGEN_CLOUDS, false, NULL);

	waterSpr = newSprite(texWidth, texHeight, 8, CEL_TYPE_CODED, waterPal, waterTex->bitmap);

	setSpritePositionZoom(waterSpr, 296, 24, 256);

	gridMesh = initGenMesh(MESH_CUBE, params, MESH_OPTIONS_DEFAULT, cloudTex);
	gridObj = initObject3D(gridMesh);
}

void effectMeshGridRun()
{
	inputScript();

	setObject3Dpos(gridObj, getMousePosition().x, -getMousePosition().y, zoom);
	setObject3Drot(gridObj, rotX, rotY, rotZ);

	renderObject3D(gridObj);

	drawSprite(waterSpr);
}
