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
	int i;
	ubyte flatCol = 255;

	flatTex = initGenTexture(64, 64, 1, flatPal, 6, TEXGEN_FLAT, true, &flatCol);
	cubeMesh = initGenMesh(256, flatTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
	setMeshDottedDisplay(cubeMesh, true);

	for (i=0; i<6; ++i) {
		cubeMesh->quad[i].palId = i;
	}
	updateMeshCELs(cubeMesh);

	draculTex = loadTexture("data/draculin.cel");
	draculMesh = initGenMesh(256, draculTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
}

void effectFeedbackOtherRun()
{
	const int time = getFrameNum();
	int z = 1024 + 768 * sin(0.25f + time * 0.01f);
	if (z > 1536) z = 1536;

	inputScript();

	setMeshPosition(cubeMesh, 0, 0, z);
	setMeshRotation(cubeMesh, time, time<<1, time>>1);
	renderMesh(cubeMesh);

	/*setMeshPosition(draculMesh, 0, 0, 512);
	setMeshRotation(draculMesh, time, time, time);
	renderMesh(draculMesh);*/
}
