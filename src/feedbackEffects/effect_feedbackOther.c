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

enum { FBO_FX_DOTCUBE1, FBO_FX_DOTCUBE2, FBO_FX_MOSAIK, FBO_FX_SLIMECUBE };

static int fbo_fx = FBO_FX_DOTCUBE2;

static Mesh *planeMesh;
static Mesh *cubeMesh;
static Mesh *draculMesh;

static Texture *flatTex;
static Texture *draculTex;
static Texture *feedbackTex0;


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
				feedbackTex0 = initFeedbackTexture(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
				//draculTex = loadTexture("data/draculin.cel");
				planeMesh = initGenMesh(256, feedbackTex0, 0, MESH_PLANE, NULL);
				setMeshDottedDisplay(planeMesh, true);
				setMeshPolygonOrder(planeMesh, true, true);
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

static void renderBufferPlane(int t, int z)
{
	setMeshPosition(planeMesh, 0, 0, z);
	setMeshRotation(planeMesh, 0, 0, 0);
	renderMesh(planeMesh);
}

void effectFeedbackOtherRun()
{
	const int time = getFrameNum();

	int z = 1024 + 768 * sin(0.25f + time * 0.01f);
	if (z > 1536) z = 1536;

	inputScript();

	switch(fbo_fx){
		case FBO_FX_DOTCUBE1:
			renderFlatCube(time, z);
		break;

		case FBO_FX_DOTCUBE2:
			switchRenderToBuffer(true);
			clearBackBuffer();
			renderFlatCube(time, 1024);

			switchRenderToBuffer(false);
			updateMeshCELs(planeMesh);
			renderBufferPlane(time, (z>>1) - 80);
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
