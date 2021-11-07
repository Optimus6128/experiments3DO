#include "core.h"

#include "effect_mosaik.h"

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

static Sprite *feedbackSpr1;
static Sprite *feedbackSpr2;

void effectMosaikInit()
{
	draculTex = loadTexture("data/draculin.cel");
	draculMesh = initGenMesh(256, draculTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
}

static void renderDraculCube(int t)
{
	setMeshPosition(draculMesh, 0, 0, 512);
	setMeshRotation(draculMesh, t, t<<1, t>>1);
	renderMesh(draculMesh);
}

void effectMosaikRun()
{
	const int time = getFrameNum();

	renderDraculCube(time);
}
