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

static Mesh *cubeMesh;
static Mesh *draculMesh;

Texture *flatTex;
Texture *draculTex;

uint16 flatPal[6] = { MakeRGB15(31,23,23), MakeRGB15(23,31,23), MakeRGB15(23,23,31), MakeRGB15(31,23,31), MakeRGB15(31,31,23), MakeRGB15(31,31,31) };

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
	ubyte flatCol = 255;

	flatTex = initGenTexture(64, 64, 1, flatPal, 6, TEXGEN_FLAT, &flatCol);
	cubeMesh = initGenMesh(256, flatTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
	setMeshDottedDisplay(cubeMesh, true);

	draculTex = loadTexture("data/draculin.cel");
	draculMesh = initGenMesh(256, draculTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
}

void effectFeedbackOtherRun()
{
	const int time = getFrameNum();

	inputScript();

	setMeshPosition(cubeMesh, 0, 0, 512);
	setMeshRotation(cubeMesh, time, time, time);
	renderMesh(cubeMesh);

	/*setMeshPosition(draculMesh, 0, 0, 512);
	setMeshRotation(draculMesh, time, time, time);
	renderMesh(draculMesh);*/
}
