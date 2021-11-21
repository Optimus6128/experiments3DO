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

typedef struct BufferRegionInfo
{
	int index;
	int posX, posY;
	int width, height;
}BufferRegionInfo;


static Mesh *draculMesh;
static Texture *draculTex;
static Sprite **feedbackLineSpr;

const int spriteLinesPerRegion = (SCREEN_HEIGHT / FRAME_SUB_Y) / 2;
const int screenRegionsNum = FRAME_SUB_X * FRAME_SUB_Y;

static int regIter = 0;
static int totalRegions;

static void initFeedbackLineSprites()
{
	int i;

	totalRegions = screenRegionsNum * getNumOffscreenBuffers();
	feedbackLineSpr = (Sprite**)AllocMem(sizeof(Sprite*) * totalRegions * spriteLinesPerRegion, MEMTYPE_ANY);

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

static BufferRegionInfo *getBufferRegionInfoFromNum(int num)
{
	static BufferRegionInfo regionInfo;

	const int regX = num % FRAME_SUB_X;
	const int regY = (num / FRAME_SUB_X) % FRAME_SUB_Y;

	regionInfo.index = num / screenRegionsNum;
	regionInfo.width = SCREEN_WIDTH / FRAME_SUB_X;
	regionInfo.height = SCREEN_HEIGHT / FRAME_SUB_Y;
	regionInfo.posX = regX * regionInfo.width;
	regionInfo.posY = regY * regionInfo.height;

	return &regionInfo;
}

void effectSlimecubeRun()
{
	const int time = getFrameNum();

	BufferRegionInfo *regionInfo = getBufferRegionInfoFromNum(regIter);

	switchRenderToBuffer(true);
	setRenderBuffer(regionInfo->index);
	setScreenRegion(regionInfo->posX, regionInfo->posY, regionInfo->width, regionInfo->height);
	clearBackBuffer();
	renderDraculCube(time);

	switchRenderToBuffer(false);
	//mapFeedbackSpriteToNewFramebufferArea(0,0, shrinkX, shrinkY, 1, feedbackLineSpr);
	drawSprite(feedbackLineSpr[regionInfo->index]);

	//drawBorderEdges(regionPosX, regionPosY, regionWidth, regionHeight);

	regIter = (regIter + 1) % totalRegions;
}
