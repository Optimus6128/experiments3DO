#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "sprite_engine.h"
#include "cel_packer.h"

#include "operamath.h"

#define SPRITE_WIDTH 64
#define SPRITE_HEIGHT 64


static Sprite *draculin;


#define PACKED_LINE_WIDTH (2 + 1+1+SPRITE_WIDTH+4)
static ubyte draculinPacked[PACKED_LINE_WIDTH * SPRITE_HEIGHT];


static void waveLeftSide(Sprite *spr, int t)
{
	int x,y;
	ubyte *src = spr->data;

	for (y=0; y<SPRITE_HEIGHT; ++y) {
		const uint16 wordWidth = *((uint16*)src);
		const int byteWidth = (wordWidth + 2) * 4;
		const int offset = (32 + (SinF16((y<<18) + (t<<17)) >> 14) + (SinF16((y<<19) + (t<<18)) >> 13)) & 63;
		*(src+2) = (PACK_TRANSPARENT << 6) | offset;
		src += byteWidth;
	}
}

static void createPackedWaveSprite(Sprite *spr)
{
	int x,y;
	ubyte *src = spr->data;
	ubyte *dst = draculinPacked;

	for (y=0; y<SPRITE_HEIGHT; ++y) {
		const uint16 wordWidth = PACKED_LINE_WIDTH / 4;
		*((uint16*)dst) = wordWidth - 2; dst += 2;

		*dst++ = (PACK_TRANSPARENT << 6);	// n=0 means 1 transparent pixel

		*dst++ = (PACK_LITERAL << 6) | (SPRITE_WIDTH - 1);	// SPRITE_WIDTH=64 pixels, aka the max per line packet
		for (x=0; x<SPRITE_WIDTH; ++x) {
			*dst++ = *src++;
		}

		*dst++ = 0;
		*dst++ = 0;
		*dst++ = 0;
		*dst++ = 0;
	}

	setSpriteData(spr, draculinPacked);

	spr->cel->ccb_Flags |= CCB_PACKED;
}


static void renderWaveSprite(Sprite *spr, int t)
{
	int posX = 160+64;// - 32 + (SinF16(t<<16) >> 10);
	int posY = 120+64;// - 32 + (CosF16(t<<17) >> 10);
	int posZ = 512 + (CosF16(t<<16) >> 7);

	setSpritePositionZoomRotate(spr, posX, posY, posZ, 64*t);

	drawSprite(spr);
}

static void render(int t)
{
	renderWaveSprite(draculin, t);

	waveLeftSide(draculin, t);
}

void effectPackedWaveInit()
{
	draculin = loadSpriteCel("data/draculin64.cel");

	createPackedWaveSprite(draculin);
}

void effectPackedWaveRun()
{
	render(getFrameNum());
}
