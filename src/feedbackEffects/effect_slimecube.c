#include "core.h"

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
static Object3D *draculObj;
static Texture *draculTex;
static Sprite **feedbackLineSpr;

static CCB *eraseCel;

const int spriteLines = (SCREEN_HEIGHT / FRAME_SUB_Y) / 2;
const int screenRegionsNum = FRAME_SUB_X * FRAME_SUB_Y;

static int regIter = 0;
static int totalRegions;
static int linesZoom = 1;

static Camera *camera;


static void scaleLineSprites(int zoom)
{
	int i;
	const int sprWidth = SCREEN_WIDTH / FRAME_SUB_X;
	const int sprHeight = 2;
	const int lineOffY = sprHeight * zoom;
	const int totalHeight = sprHeight * spriteLines;
	const int ulX = (SCREEN_WIDTH - zoom * sprWidth) >> 1;
	int ulY = (SCREEN_HEIGHT - zoom * totalHeight) >> 1;

	for (i=0; i<spriteLines; ++i) {
		mapZoomSpriteToQuad(feedbackLineSpr[i], ulX, ulY, ulX + zoom * sprWidth, ulY + lineOffY);
		ulY += lineOffY;
	}
}

static void initFeedbackLineSprites()
{
	int i;
	feedbackLineSpr = (Sprite**)AllocMem(sizeof(Sprite*) * spriteLines, MEMTYPE_ANY);

	for (i=0; i<spriteLines; ++i) {
		feedbackLineSpr[i] = newFeedbackSprite(0, 0, SCREEN_WIDTH / FRAME_SUB_X, 2, 0);
		if (i>0) linkCel(feedbackLineSpr[i-1]->cel, feedbackLineSpr[i]->cel);
	}

	scaleLineSprites(1);
}

void effectSlimecubeInit()
{
	initFeedbackLineSprites();

	totalRegions = screenRegionsNum * getNumOffscreenBuffers();

	draculTex = loadTexture("data/draculin64.cel");
	draculMesh = initGenMesh(MESH_CUBE, DEFAULT_MESHGEN_PARAMS(256), MESH_OPTIONS_DEFAULT, draculTex);
	draculObj = initObject3D(draculMesh);

	eraseCel = CreateBackdropCel(SCREEN_WIDTH / FRAME_SUB_X, SCREEN_HEIGHT / FRAME_SUB_Y, 0, 100);
	eraseCel->ccb_Flags |= CCB_BGND;

	camera = createCamera();
}

static void renderDraculCube(int t)
{
	setObject3Dpos(draculObj, 0, 0, 1408);	//4x3
	//setObject3Dpos(draculObj, 0, 0, 960);	// 2x2
	setObject3Drot(draculObj, t, t<<1, t>>1);
	renderObject3D(draculObj, camera, NULL, 0);
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

static void inputScript()
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_A)) {
		if(++linesZoom>3) linesZoom = 1;
		scaleLineSprites(linesZoom);
	}
}

static int getBackInTimeIter(int presentIter, int line, int t)
{
	const int waveAmp = totalRegions >> 2;
	const int wave = (((SinF16((3*line+2*t) << 16) + 65536) * waveAmp) >> 16) + (((SinF16((2*line-t) << 16) + 65536) * waveAmp) >> 16);

	int pastIter = presentIter - wave;
	if (pastIter < 0) pastIter += totalRegions;

	return pastIter;
}

void effectSlimecubeRun()
{
	int i;
	const int time = getFrameNum();

	BufferRegionInfo *regionInfo = getBufferRegionInfoFromNum(regIter);

	inputScript();

	switchRenderToBuffer(true);
	setRenderBuffer(regionInfo->index);
	setScreenRegion(regionInfo->posX, regionInfo->posY, regionInfo->width, regionInfo->height);

	eraseCel->ccb_XPos = regionInfo->posX << 16;
	eraseCel->ccb_YPos = regionInfo->posY << 16;
	drawCels(eraseCel);

	renderDraculCube(time);

	for (i=0; i<spriteLines; ++i) {
		int posX, posY;
		regionInfo = getBufferRegionInfoFromNum(getBackInTimeIter(regIter, i, time));

		posX = regionInfo->posX;
		posY = regionInfo->posY + (i<<1);
		mapFeedbackSpriteToNewFramebufferArea(posX, posY, posX + regionInfo->width, posY + 2, regionInfo->index, feedbackLineSpr[i]);
	}

	switchRenderToBuffer(false);
	drawSprite(feedbackLineSpr[0]);

	//drawBorderEdges(regionPosX, regionPosY, regionWidth, regionHeight);

	regIter = (regIter + 1) % totalRegions;
}
