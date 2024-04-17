#include "core.h"

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

#define CUBE_BUFFER_WIDTH 128
#define CUBE_BUFFER_HEIGHT 128


enum { FBO_FX_DOTCUBE1, FBO_FX_DOTCUBE2, FBO_FX_NUM };

static bool fboFxInit[FBO_FX_NUM] = { false, false };
static char *partName[FBO_FX_NUM] = { "REAL 3D DOTCUBE", "DESERT DREAM CUBE" };

static int fbo_fx = FBO_FX_DOTCUBE2;

static Mesh *cubeMesh;
static Object3D *cubeObj;

static Camera *camera;

static Texture *flatTex;
static Sprite *feedbackSpr;

static uint16 flatPal[12] = { 0, MakeRGB15(31,23,23), 0, MakeRGB15(23,31,23), 0, MakeRGB15(23,23,31), 0, MakeRGB15(31,23,31), 0, MakeRGB15(31,31,23), 0, MakeRGB15(31,31,31) };


static void setupDotcubeFx()
{
	switch(fbo_fx){
		case FBO_FX_DOTCUBE1:
		case FBO_FX_DOTCUBE2:
		{
			int i;
			unsigned char flatCol = 255;

			flatTex = initGenTexture(64, 64, 1, flatPal, 6, TEXGEN_FLAT, &flatCol);
			cubeMesh = initGenMesh(MESH_CUBE, DEFAULT_MESHGEN_PARAMS(256), MESH_OPTIONS_DEFAULT | MESH_OPTION_NO_POLYCLIP, flatTex);
			cubeObj = initObject3D(cubeMesh);

			for (i=0; i<6; ++i) {
				cubeMesh->poly[i].palId = i;
			}
			updateMeshCELs(cubeMesh);

			if (fbo_fx==FBO_FX_DOTCUBE2) {
				feedbackSpr = newFeedbackSprite(0, 0, CUBE_BUFFER_WIDTH, CUBE_BUFFER_HEIGHT, 0);
				setSpriteDottedDisplay(feedbackSpr, true);
			}
		}
		break;

		default:
		break;
	}
}

void effectDotcubeInit()
{
	setupDotcubeFx();

	camera = createCamera();
}

static void partChanged()
{
	if (!fboFxInit[fbo_fx]) {
		setupDotcubeFx();
		fboFxInit[fbo_fx] = true;
	}
	setMeshDottedDisplay(cubeMesh, (fbo_fx==FBO_FX_DOTCUBE1));
}

static void inputScript()
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_LPAD)) {
		--fbo_fx;
		if (fbo_fx < 0) fbo_fx = FBO_FX_DOTCUBE2;
		partChanged();
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_RPAD)) {
		++fbo_fx;
		if (fbo_fx > FBO_FX_DOTCUBE2) fbo_fx = 0;
		partChanged();
	}
}

static void renderFlatCube(int t, int z)
{
	setObject3Dpos(cubeObj, 0, 0, z);
	setObject3Drot(cubeObj, t, t<<1, t>>1);
	renderObject3D(cubeObj, camera, NULL, 0);
}

void effectDotcubeRun()
{
	const int time = getFrameNum();

	inputScript();

	switch(fbo_fx){
		case FBO_FX_DOTCUBE1:
		{
			int z = (int)(1024 + 768 * sin(0.25f + time * 0.01f));
			if (z > 1536) z = 1536;

			renderFlatCube(time, z);
		}
		break;

		case FBO_FX_DOTCUBE2:
		{
			int z = (int)((1.0f - cos(time * 0.01f)) * 2048);
			if (z < 256) z = 256;

			switchRenderToBuffer(true);
			setScreenRegion(0, 0, CUBE_BUFFER_WIDTH, CUBE_BUFFER_HEIGHT);
			clearBackBuffer();
			renderFlatCube(time, 1024);

			switchRenderToBuffer(false);
			setScreenRegion(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
			setSpritePositionZoom(feedbackSpr, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, z);
			drawSprite(feedbackSpr);

			//drawBorderEdges(0,0, CUBE_BUFFER_WIDTH,CUBE_BUFFER_HEIGHT);
		}
		break;

		default:
		break;
	}
	drawText(8, SCREEN_HEIGHT - 16, partName[fbo_fx]);
}
