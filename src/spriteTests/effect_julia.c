#include "core.h"

#include "effect_julia.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"

#include "mathutil.h"

#include "input.h"


#define JULIA_WIDTH 240
#define JULIA_HEIGHT 160

static Sprite *juliaSpr;
static uint16 juliaPal[16];
static ubyte *juliaBmp;


static void juliaRender()
{
	int x,y;

	ubyte *dst = juliaBmp;
	for (y=0; y<JULIA_HEIGHT; ++y) {
		for (x=0; x<JULIA_WIDTH; x+=2) {
			int c0 = (x^y) & 15;
			int c1 = ((x+1)^y) & 15;
			*dst++ = (c0 << 4) | c1;
		}
	}
}


void effectJuliaInit()
{
	juliaBmp = (uint8*)AllocMem((JULIA_WIDTH * JULIA_HEIGHT) / 2, MEMTYPE_ANY);
	juliaSpr = newSprite(JULIA_WIDTH, JULIA_HEIGHT, 4, CEL_TYPE_CODED, juliaPal, juliaBmp);

	setPalGradient(0,15, 0,0,1, 31,31,31, juliaPal);
	setSpritePosition(juliaSpr, (SCREEN_WIDTH - JULIA_WIDTH) / 2, (SCREEN_HEIGHT - JULIA_HEIGHT) / 2);
}

void effectJuliaRun()
{
	juliaRender();

	drawSprite(juliaSpr);
}
