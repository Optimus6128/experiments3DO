#include <math.h>

#include "core.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"

#include "mathutil.h"

#include "input.h"


#define SPHERE_WIDTH 96
#define SPHERE_HEIGHT 96
#define SPHERE_RADIUS (SPHERE_WIDTH/2)

#define SPHERE_PIXEL_TRANSPARENT 32767

uint8 *background;
int *spheremap;

uint8 xOffset[SPHERE_HEIGHT];
uint8 xLength[SPHERE_HEIGHT];

static Sprite *sphereFxSpr;
static uint16 spherePal[32];
static unsigned char *sphereBmp;

static bool showBg = true;


static void renderSphere(int ulOffset)
{
	int x,y;

	uint8 *src = background + ulOffset;
	uint8 *dst = (uint8*)sphereBmp;
	int *spr = spheremap;

	for (y=0; y<SPHERE_HEIGHT; ++y) {
		const uint8 x0 = xOffset[y];
		const uint8 x1 = x0+xLength[y];
		for (x=x0; x<x1; ++x) {
			int offset = *spr++;
			*(dst+x) = *(src + offset);
		}
		dst += SPHERE_WIDTH;
	}
}

static void initSphere()
{
	int x,y;

	int *dst = spheremap;

	int yi = 0;
	for (y=-SPHERE_HEIGHT/2; y<SPHERE_HEIGHT/2; ++y) {
		const int yc = y*y;
		xOffset[yi] = 0;
		xLength[yi] = 0;
		for (x=-SPHERE_WIDTH/2; x<SPHERE_WIDTH/2; ++x) {
			const int xc = x*x;
			const int z1 = SPHERE_RADIUS * SPHERE_RADIUS - xc - yc - 1;
			const int z2 = (SPHERE_RADIUS - 3) * (SPHERE_RADIUS - 3) - xc - yc - 1;
			if (z2 < 0) {
				if (xLength[yi]==0) xOffset[yi]++;
			} else {
				float z = (float)sqrt((float)z1 / 256.0f);
				if (z < 1.0f) z = 1.0f;
				*dst++ = ((int)(y / z) + SPHERE_HEIGHT/2) * SCREEN_WIDTH + (int)(x / z) + SPHERE_WIDTH / 2;
				xLength[yi]++;
			}
		}
		++yi;
	}
}

static void prepareBackgrounds()	// One for SPORT write and a degraded 8bit second one for sphere lookup
{
	int x,y,i;
	uint16 *bg16 = (uint16*)getBackBuffer();

	loadAndSetBackgroundImage("data/background.img", bg16);

	for (y=0; y<SCREEN_HEIGHT; y+=2) {
		for (x=0; x<SCREEN_WIDTH; ++x) {
			for (i=0; i<2; ++i) {
				const uint16 c = *bg16++;
				const int r = (c >> (12 + 0)) & 7;
				const int g = (c >> (7 + 0)) & 7;
				const int b = ((c >> 3) & 3) | 2;
				*(background+(y+i)*SCREEN_WIDTH+x) = (r << 5) | (g << 2) | b;
			}
		}
	}
}


static void inputScript()
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_A)) {
		showBg = !showBg;
	}
}

void effectSphereInit()
{
	const int size = SPHERE_WIDTH * SPHERE_HEIGHT;

	sphereBmp = (unsigned char*)AllocMem(size, MEMTYPE_ANY);
	memset(sphereBmp, 0, size);
	sphereFxSpr = newSprite(SPHERE_WIDTH, SPHERE_HEIGHT, 8, CEL_TYPE_UNCODED, spherePal, sphereBmp);

	background = (uint8*)AllocMem(SCREEN_WIDTH * SCREEN_HEIGHT, MEMTYPE_ANY);
	spheremap = (int*)AllocMem(SPHERE_WIDTH * SPHERE_HEIGHT * sizeof(int), MEMTYPE_ANY);

	prepareBackgrounds();

	initSphere();
}

static void moveAndRenderSphere()
{
	const int t = getTicks();
	const int xp = SCREEN_WIDTH / 2 + (int)(sin(t/1132.0) * 64) - SPHERE_WIDTH/2;
	const int yp = SCREEN_HEIGHT / 2 + (int)(sin(t/948.0) * 48) - SPHERE_HEIGHT/2;

	const uint16 ulOffset = yp * SCREEN_WIDTH + xp;

	renderSphere(ulOffset);

	setSpritePosition(sphereFxSpr, xp, yp);
	drawSprite(sphereFxSpr);
}

void effectSphereRun()
{
	inputScript();

	if (showBg) {
		switchToSPORTimage();
	} else {
		switchToSPORTwrite();
	}

	moveAndRenderSphere();
}
