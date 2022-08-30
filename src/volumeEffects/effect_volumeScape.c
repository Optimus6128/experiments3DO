#include "core.h"

#include "effect_volumeScape.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"
#include "input.h"
#include "mathutil.h"
#include "file_utils.h"


#define HMAP_WIDTH 1024
#define HMAP_HEIGHT 1024
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)


static uint8 *hmap;
static uint8 *cmap;
//static Sprite *testSpr;

static CCB *columnCels;
static uint8 *columnPixels;
static uint16 columnPal[32];
static uint8 yMax[SCREEN_WIDTH];

static void fillColumnPixelsTest()
{
	int i;
	for (i=0; i<SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
		columnPixels[i] = getRand(0,31);
	}

	for (i=0; i<SCREEN_WIDTH; ++i) {
		yMax[i] = 16 + ((i * i) & 127);
	}
}

static void updateColumnHeights()
{
	int i;
	for (i=0; i<SCREEN_WIDTH; ++i) {
		setCelWidth(yMax[i], &columnCels[i]);
	}
}

static void loadHeightmap()
{
	int i;

	hmap = AllocMem(HMAP_SIZE, MEMTYPE_ANY);
	//cmap = AllocMem(HMAP_SIZE/4, MEMTYPE_ANY);
	columnPixels = (uint8*)AllocMem(SCREEN_WIDTH * SCREEN_HEIGHT, MEMTYPE_ANY);

	readBytesFromFileAndStore("data/hmap1.bin", 0, HMAP_SIZE, hmap);
	//testSpr = newSprite(HMAP_WIDTH, HMAP_HEIGHT, 8, CEL_TYPE_UNCODED, NULL, hmap);
	//setSpritePositionZoom(testSpr, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 256);

	columnCels = createCels(SCREEN_HEIGHT, 1, 8, CEL_TYPE_CODED, SCREEN_WIDTH);

	setPalGradient(0,31, 0,0,0, 31,31,31, columnPal);

	for (i=0; i<SCREEN_WIDTH; ++i) {
		CCB *cel = &columnCels[i];

		setupCelData(columnPal, &columnPixels[i * SCREEN_HEIGHT], cel);
		setCelPosition(i, SCREEN_HEIGHT-1, cel);
		rotateCelOrientation(cel);
		flipCelOrientation(true, false, cel);

		if (i>0) linkCel(&columnCels[i-1], cel);
	}

	fillColumnPixelsTest();
	updateColumnHeights();
}

void effectVolumeScapeInit()
{
	loadHeightmap();
}

static void updateFromInput()
{
	if (isJoyButtonPressed(JOY_BUTTON_A)) {
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
	}
	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_B)) {
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
	}
}

void effectVolumeScapeRun()
{
	updateFromInput();

	drawCels(&columnCels[0]);
	//drawSprite(testSpr);
}
