#include "core.h"

#include "effect_packedRain.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "mathutil.h"
#include "sprite_engine.h"
#include "cel_packer.h"
#include "engine_main.h"


#define RAIN_LAYER_WIDTH 256
#define RAIN_LAYER_HEIGHT 256
#define RAIN_LAYER_SIZE (RAIN_LAYER_WIDTH * RAIN_LAYER_HEIGHT)
#define RAIN_LAYERS_NUM 32


static Sprite *unpackedRain[RAIN_LAYERS_NUM];
static Sprite *packedRain[RAIN_LAYERS_NUM];
static unsigned char *unpackedRainBmp[RAIN_LAYERS_NUM];
static uint16 pal2[4];

static bool showPackedRain = false;
static bool showPackInfo = false;

static int totalPackedPercentage = 0;

/*static void writePixel2bpp(int posX, int posY, unsigned char *dst, int color)
{
}

static void writeRainLine(int posX, int posY, unsigned char *dst, int dirX)
{
}*/

static void generateUnpackedRain()
{
	int i, j;
	
	for (i=0; i<RAIN_LAYERS_NUM; ++i) {
		unsigned char *dst = (unsigned char*)AllocMem(RAIN_LAYER_SIZE / 4, MEMTYPE_ANY);
		unpackedRainBmp[i] = dst;

		for (j=0; j<128; ++j) {
			// noise test for now (will use the two above functions later)
			const int bx = getRand(0, RAIN_LAYER_WIDTH / 4 - 1);
			const int by = getRand(0, RAIN_LAYER_HEIGHT - 1);
			*(dst + by * (RAIN_LAYER_WIDTH / 4) + bx) = (unsigned char)getRand(0, 255);
		}
	}

	pal2[0] = 0;
	setPalGradient(1,3, 20,24,31, 31,31,31, pal2);
}

void effectPackedRainInit()
{
	int i;

	initCelPackerEngine();
	
	//loadAndSetBackgroundImage("data/background.cel", NULL);
	//Bugs memory if on. Lib3DO function bug again? Let's try giving my own pointer next.
	//Loading it after generateUnpackedRain works though. Maybe generateUnpackedRain has memory leak?

	generateUnpackedRain();

	for (i=0; i<RAIN_LAYERS_NUM; ++i) {
		unpackedRain[i] = newSprite(RAIN_LAYER_WIDTH, RAIN_LAYER_HEIGHT, 2, CEL_TYPE_CODED, pal2, unpackedRainBmp[i]);
		packedRain[i] = newPackedSprite(RAIN_LAYER_WIDTH, RAIN_LAYER_HEIGHT, 2, CEL_TYPE_CODED, pal2, unpackedRainBmp[i], NULL, 0);
		totalPackedPercentage += packPercentage;
	}

	deinitCelPackerEngine();
}

static void updateFromInput()
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_A)) showPackedRain = !showPackedRain;
	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) showPackInfo = !showPackInfo;
}

static void renderZoomRotateRainLayerSprite(Sprite *spr, int offsetX, int offsetY, int zoom, int angle)
{
	setSpritePositionZoomRotate(spr, SCREEN_WIDTH / 2 + offsetX, SCREEN_HEIGHT / 2 + offsetY, zoom, angle);

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
	const int layersOnScreen = RAIN_LAYERS_NUM;
	const int time = getFrameNum();
	const int angle = (64 << 8) + ((SinF16(time * 30000) << 3) >> 8);

	int i;
	for (i=0; i<layersOnScreen; ++i) {
		const int zoom = 512 - ((layersOnScreen - i) * 448) / layersOnScreen + ((2*time) & 127);
		const int fade = shadeTable[i & (SHADE_TABLE_SIZE-1)];

		if (showPackedRain) {
			packedRain[i]->cel->ccb_PIXC = fade;
			renderZoomRotateRainLayerSprite(packedRain[i], 0, 0, zoom, angle);
		}
		else {
			unpackedRain[i]->cel->ccb_PIXC = fade;
			renderZoomRotateRainLayerSprite(unpackedRain[i], 0, 0, zoom, angle);
		}
	}
}

static void render()
{
	renderRain();

	renderText();
}

void effectPackedRainRun()
{
	updateFromInput();

	render();
}
