#include "core.h"

#include "effect_slimecube.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"

#include "procgen_mesh.h"

#include "sprite_engine.h"

static Mesh *draculMesh;
static Texture *draculTex;
static Sprite *feedbackLineSpr[4*4*4][30]; // 4 Backbuffers, (4*80)*(4*60) = 320*240, line can be minimum 2 scanlines (because of vram structure), 60/2 = 30

void effectSlimecubeInit()
{
	draculTex = loadTexture("data/draculin64.cel");
	draculMesh = initGenMesh(256, draculTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
}

static void renderDraculCube(int t)
{
	setMeshPosition(draculMesh, 0, 0, 512);
	setMeshRotation(draculMesh, t, t<<1, t>>1);
	renderMesh(draculMesh);
}

void effectSlimecubeRun()
{
	const int time = getFrameNum();

	renderDraculCube(time);
}
