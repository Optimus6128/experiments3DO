#include "core.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"

#include "mathutil.h"

#include "input.h"


#define WATER_FX_WIDTH 320
#define WATER_FX_HEIGHT 200
#define WATER_WIDTH_WORDS (WATER_FX_WIDTH / 4)

static Sprite *waterFxSpr;
static uint16 waterPal[32];
static unsigned char *waterBmp;

static unsigned char *waterBuffer1;
static unsigned char *waterBuffer2;

unsigned char *wb1, *wb2;

static void swapBuffers()
{
	unsigned char *temp = wb2;
	wb2 = wb1;
	wb1 = temp;
}

static void updateWater(unsigned char *buffer1, unsigned char *buffer2, unsigned char *vramOffset)
{
	int count = WATER_WIDTH_WORDS * (WATER_FX_HEIGHT - 2) - 2;

	unsigned int *src1 = (unsigned int*)buffer1;
	unsigned int *src2 = (unsigned int*)buffer2;
	unsigned int *vram = (unsigned int*)vramOffset;

	do {
		const unsigned int cb_0 = *((unsigned char*)src1-1);
		const unsigned int cb_1234 = *src1;
		const unsigned int cb_5 = *((unsigned char*)src1+4);
		const unsigned int c0 = (cb_0 << 24) | (cb_1234 >> 8);
		const unsigned int c1 = (cb_1234 << 8) | cb_5;
		const unsigned int c2 = *(src1-WATER_WIDTH_WORDS);
		const unsigned int c3 = *(src1+WATER_WIDTH_WORDS);

		// Water effect on 4 bytes packed in 32bits
		const unsigned int c = (((c0 + c1 + c2 + c3) >> 1) & 0x7f7f7f7f) - *src2;

		// Do absolute value of 4 bytes packed in 32bits (From Hacker's Delight)
		const unsigned int a = c & 0x80808080;
		const unsigned int b = a >> 7;
		const unsigned int cc = (c ^ ((a - b) | a)) + b;

		src1++;
		*src2++ = cc;
		*vram++ = (cc >> 1) & 0x1f1f1f1f;
	} while (--count > 0);
}

static void renderBlob(int xp, int yp, int width, unsigned char *buffer)
{
	int i,j;
	unsigned char *dst = buffer + yp * width + xp;

	for (j=0; j<3; ++j) {
		for (i=0; i<3; ++i) {
			*(dst + i) = 0x3f;
		}
		dst += width;
	}
}

static void makeRipples()
{
	//int i;
	renderBlob(32 + (rand() & 255), 32 + (rand() & 127), WATER_FX_WIDTH,wb1);

	/*for (i=0; i<3; ++i) {
		const int t = getTicks();
		const int xp = WATER_FX_WIDTH / 2 + (int)(sin((t + 16384*i)/724.0) * 64);
		const int yp = WATER_FX_HEIGHT / 2 + (int)(sin((t + 16384*i)/816.0) * 48);

		renderBlob(xp,yp,WATER_FX_WIDTH,wb1);
	}*/
}

void effectWaterInit()
{
	const int size = WATER_FX_WIDTH * WATER_FX_HEIGHT;

	waterBmp = (unsigned char*)AllocMem(size, MEMTYPE_ANY);
	memset(waterBmp, 0, size);
	waterFxSpr = newSprite(WATER_FX_WIDTH, WATER_FX_HEIGHT, 8, CEL_TYPE_CODED, waterPal, waterBmp);

	setPalGradient(0,31, 0,0,1, 31,31,31, waterPal);
	setSpritePosition(waterFxSpr, (SCREEN_WIDTH - WATER_FX_WIDTH) / 2, (SCREEN_HEIGHT - WATER_FX_HEIGHT) / 2);

	waterBuffer1 = (unsigned char*)AllocMem(size, MEMTYPE_ANY);
	waterBuffer2 = (unsigned char*)AllocMem(size, MEMTYPE_ANY);
	memset(waterBuffer1, 0, size);
	memset(waterBuffer2, 0, size);

	wb1 = waterBuffer1;
	wb2 = waterBuffer2;
}

void effectWaterRun()
{
	unsigned char *buff1 = wb1 + WATER_FX_WIDTH + 4;
	unsigned char *buff2 = wb2 + WATER_FX_WIDTH + 4;
	unsigned char *vramOffset = waterBmp + WATER_FX_WIDTH + 4;

	makeRipples();

	updateWater(buff1, buff2, vramOffset);

	swapBuffers();

	drawSprite(waterFxSpr);
}
