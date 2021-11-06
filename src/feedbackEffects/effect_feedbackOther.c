#include "core.h"

#include "effect_feedbackOther.h"

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


enum { FBO_FX_DOTCUBE1, FBO_FX_DOTCUBE2, FBO_FX_MOSAIK, FBO_FX_SLIMECUBE };

#define CUBE_BUFFER_WIDTH 128
#define CUBE_BUFFER_HEIGHT 128

static int fbo_fx = FBO_FX_DOTCUBE2;

static Mesh *cubeMesh;
static Mesh *draculMesh;

static Texture *flatTex;
static Texture *draculTex;
static Sprite *feedbackSpr;

uint16 flatPal[12] = { 0, MakeRGB15(31,23,23), 0, MakeRGB15(23,31,23), 0, MakeRGB15(23,23,31), 0, MakeRGB15(31,23,31), 0, MakeRGB15(31,31,23), 0, MakeRGB15(31,31,31) };


static void inputScript()
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_A)) {
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_B)) {
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_LPAD)) {
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_RPAD)) {
	}
}

void effectFeedbackOtherInit()
{
	switch(fbo_fx){
		case FBO_FX_DOTCUBE1:
		case FBO_FX_DOTCUBE2:
		{
			int i;
			ubyte flatCol = 255;

			flatTex = initGenTexture(64, 64, 1, flatPal, 6, TEXGEN_FLAT, true, &flatCol);
			cubeMesh = initGenMesh(256, flatTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
			setMeshDottedDisplay(cubeMesh, (fbo_fx==FBO_FX_DOTCUBE1));

			for (i=0; i<6; ++i) {
				cubeMesh->quad[i].palId = i;
			}
			updateMeshCELs(cubeMesh);

			if (fbo_fx==FBO_FX_DOTCUBE2) {
				feedbackSpr = newFeedbackSprite(0, 0, CUBE_BUFFER_WIDTH, CUBE_BUFFER_HEIGHT, 0);
				setSpriteDottedDisplay(feedbackSpr, true);
			}
		}
		break;

		case FBO_FX_MOSAIK:
		case FBO_FX_SLIMECUBE:
			draculTex = loadTexture("data/draculin.cel");
			draculMesh = initGenMesh(256, draculTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
		break;

		default:
		break;
	}
}

static void renderDraculCube(int t)
{
	setMeshPosition(draculMesh, 0, 0, 512);
	setMeshRotation(draculMesh, t, t<<1, t>>1);
	renderMesh(draculMesh);
}

static void renderFlatCube(int t, int z)
{
	setMeshPosition(cubeMesh, 0, 0, z);
	setMeshRotation(cubeMesh, t, t<<1, t>>1);
	renderMesh(cubeMesh);
}

void effectFeedbackOtherRun()
{
	const int time = getFrameNum();

	inputScript();

	switch(fbo_fx){
		case FBO_FX_DOTCUBE1:
		{
			int z = 1024 + 768 * sin(0.25f + time * 0.01f);
			if (z > 1536) z = 1536;

			renderFlatCube(time, z);
		}
		break;

		case FBO_FX_DOTCUBE2:
		{
			int z = (1.0f - cos(time * 0.01f)) * 2048;
			if (z < 256) z = 256;

			switchRenderToBuffer(true);
			setScreenDimensions(CUBE_BUFFER_WIDTH, CUBE_BUFFER_HEIGHT);
			clearBackBuffer();
			renderFlatCube(time, 1024);

			switchRenderToBuffer(false);
			setScreenDimensions(SCREEN_WIDTH, SCREEN_HEIGHT);
			setSpritePositionZoom(feedbackSpr, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, z);
			drawSprite(feedbackSpr);

			//drawBorderEdges(0,0, CUBE_BUFFER_WIDTH,CUBE_BUFFER_HEIGHT);
		}
		break;

		case FBO_FX_MOSAIK:
			renderDraculCube(time);
		break;

		case FBO_FX_SLIMECUBE:
			renderDraculCube(time);
		break;

		default:
		break;
	}
}
