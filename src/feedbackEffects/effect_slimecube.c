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


#define FRAME_SUB_X 4
#define FRAME_SUB_Y 3

static Mesh *draculMesh;
static Texture *draculTex;
static Sprite **feedbackLineSpr;

const int spriteLinesPerRegion = (SCREEN_HEIGHT / FRAME_SUB_Y) / 2;
const int screenRegionsNum = FRAME_SUB_X * FRAME_SUB_Y;

static int regIndex = 0;
static int regBuffIndex = 0;

static void initFeedbackLineSprites()
{
	int i;
	const int numOfLineSprites = screenRegionsNum * getNumOffscreenBuffers() * spriteLinesPerRegion;

	feedbackLineSpr = (Sprite**)AllocMem(sizeof(Sprite*) * numOfLineSprites, MEMTYPE_ANY);

	// Testing with fullscreen buffers, not the final solution
	for (i=0; i<getNumOffscreenBuffers(); ++i) {
		feedbackLineSpr[i] = newFeedbackSprite(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, i);
	}
}

void effectSlimecubeInit()
{
	initFeedbackLineSprites();

	draculTex = loadTexture("data/draculin64.cel");
	draculMesh = initGenMesh(256, draculTex, MESH_OPTIONS_DEFAULT, MESH_CUBE, NULL);
}

static void renderDraculCube(int t)
{
	setMeshPosition(draculMesh, 0, 0, 1408);
	setMeshRotation(draculMesh, t, t<<1, t>>1);
	renderMesh(draculMesh);
}

static void cycleRegions()
{
	++regIndex;
	if (regIndex==screenRegionsNum) {
		regIndex = 0;
		++regBuffIndex;
		if (regBuffIndex==getNumOffscreenBuffers()) {
			regBuffIndex = 0;
		}
	}
}

void effectSlimecubeRun()
{
	const int time = getFrameNum();

	const int regX = regIndex % FRAME_SUB_X;
	const int regY = (regIndex / FRAME_SUB_X) % FRAME_SUB_Y;

	const int regionWidth = SCREEN_WIDTH / FRAME_SUB_X;
	const int regionHeight = SCREEN_HEIGHT / FRAME_SUB_Y;
	const int regionPosX = regX * regionWidth;
	const int regionPosY = regY * regionHeight;

	switchRenderToBuffer(true);
	setRenderBuffer(regBuffIndex);
	setScreenRegion(regionPosX, regionPosY, regionWidth, regionHeight);
	//clearBackBuffer();
	renderDraculCube(time);

	switchRenderToBuffer(false);
	//mapFeedbackSpriteToNewFramebufferArea(0,0, shrinkX, shrinkY, 1, feedbackLineSpr);
	drawSprite(feedbackLineSpr[regBuffIndex]);

	//drawBorderEdges(regionPosX, regionPosY, regionWidth, regionHeight);

	cycleRegions();
}
