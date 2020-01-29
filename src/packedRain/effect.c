#include "main.h"
#include "tools.h"

#include "effect.h"

#include "system_graphics.h"
#include "input.h"

#include "sprite_engine.h"
#include "cel_packer.h"


#define RAIN_LAYER_WIDTH 256
#define RAIN_LAYER_HEIGHT 256
#define RAIN_LAYER_SIZE (RAIN_LAYER_WIDTH * RAIN_LAYER_HEIGHT)
#define RAIN_LAYERS_NUM 32

static Sprite *unpackedRain[RAIN_LAYERS_NUM];
static Sprite *packedRain[RAIN_LAYERS_NUM];
static ubyte *unpackedRainBmp[RAIN_LAYERS_NUM];
static ubyte *packedRainData[RAIN_LAYERS_NUM];
static uint16 pal2[4];

static bool showPackedRain = false;
static bool showPackInfo = false;

static int totalPackedPercentage = 0;

static void writePixel2bpp(int posX, int posY, ubyte *dst, int color)
{
}

static void writeRainLine(int posX, int posY, ubyte *dst, int dirX)
{
}

static void generateUnpackedRain()
{
	int i, j;
	
	for (i=0; i<RAIN_LAYERS_NUM; ++i) {
		ubyte *dst = (ubyte*)AllocMem(RAIN_LAYER_SIZE / 4, MEMTYPE_ANY);
		unpackedRainBmp[i] = dst;

		for (j=0; j<128; ++j) {
			// noise test for now (will use the two above functions later)
			const int bx = rand() & (RAIN_LAYER_WIDTH / 4 - 1);
			const int by = rand() & (RAIN_LAYER_HEIGHT - 1);
			*(dst + by * (RAIN_LAYER_WIDTH / 4) + bx) = (ubyte)rand();
		}
	}

	pal2[0] = 0;
	setPal(1,3, 160,192,255, 255,255,255, pal2);
}

void effectInit()
{
	int i;

	generateUnpackedRain();

	for (i=0; i<RAIN_LAYERS_NUM; ++i) {
		unpackedRain[i] = newSprite(RAIN_LAYER_WIDTH, RAIN_LAYER_HEIGHT, 2, CREATECEL_CODED, pal2, unpackedRainBmp[i]);
		packedRainData[i] = NULL;
		packedRain[i] = newPackedSprite(RAIN_LAYER_WIDTH, RAIN_LAYER_HEIGHT, 2, CREATECEL_CODED, pal2, unpackedRainBmp[i], packedRainData[i]);
		totalPackedPercentage += packPercentage;
	}
}

static void updateFromInput()
{
	if (isButtonPressedOnce(BUTTON_A)) showPackedRain = !showPackedRain;
	if (isButtonPressedOnce(BUTTON_START)) showPackInfo = !showPackInfo;
}

static void renderZoomRotateRainLayerSprite(Sprite *spr, int offsetX, int offsetY, int zoom, int angle)
{
	spr->angle = angle;
	spr->zoom = zoom;
	spr->posX = SCREEN_WIDTH / 2 + offsetX;
	spr->posY = SCREEN_HEIGHT / 2 + offsetY;

	mapZoomRotateSprite(spr);

	drawSprite(spr);
}

static void renderText()
{
	if (showPackedRain)
		drawText(192, 8, "Packed");
	else
		drawText(192, 8, "Unpacked");

	if (showPackInfo) drawNumber(8, 232, packPercentage);
}

static void renderRain()
{
	int i;
	const int layersOnScreen = RAIN_LAYERS_NUM;
	const int angle = (64 << 16) + (SinF16(time * 30000) << 3);	// 90 degrees

	for (i=0; i<layersOnScreen; ++i) {
		const int zoom = 512 - ((layersOnScreen - i) * 448) / layersOnScreen + ((2*time) & 127);
		const int k = i;//rand() % layersOnScreen;
		const int fade = (0x1F * (i + 1)) / layersOnScreen;

		if (showPackedRain) {
			packedRain[k]->cel->ccb_PIXC = (fade << 8) | 0x80;
			renderZoomRotateRainLayerSprite(packedRain[k], 0, 0, zoom, angle);
		}
		else {
			unpackedRain[k]->cel->ccb_PIXC = (fade << 8) | 0x80;
			renderZoomRotateRainLayerSprite(unpackedRain[k], 0, 0, zoom, angle);
		}
	}
}

static void render()
{
	renderRain();

	renderText();
}

void effectRun()
{
	updateFromInput();

	render();
}
