#include "core.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "sprite_engine.h"
#include "cel_packer.h"


#define ENABLE_4 true
#define ENABLE_8 true
#define ENABLE_16 true


#define SPRITE4_WIDTH 512
#define SPRITE4_HEIGHT 512
#define SPRITE4_SIZE (SPRITE4_WIDTH * SPRITE4_HEIGHT)

#define SPRITE8_WIDTH 128
#define SPRITE8_HEIGHT 128
#define SPRITE8_SIZE (SPRITE8_WIDTH * SPRITE8_HEIGHT)

#define SPRITE16_WIDTH 384
#define SPRITE16_HEIGHT 240
#define SPRITE16_SIZE (SPRITE16_WIDTH * SPRITE16_HEIGHT)


static Sprite *unpackedSpr4;
static Sprite *packedSpr4;
static unsigned char unpackedBmp4[SPRITE4_SIZE/2];
static uint16 pal4[16];
static uint16 pal8[32];

static Sprite *unpackedSpr8;
static Sprite *packedSpr8;
static unsigned char unpackedBmp8[SPRITE8_SIZE];

static Sprite *unpackedSpr16;
static Sprite *packedSpr16;
static uint16 unpackedBmp16[SPRITE16_SIZE];

static bool showPacked4 = false;
static bool showPacked8 = false;
static bool showPacked16 = false;
static bool showPackInfo = false;

static bool showRender4 = ENABLE_4;
static bool showRender8 = ENABLE_8;
static bool showRender16 = ENABLE_16;

static int packedPer4;
static int packedPer8;
static int packedPer16;

static void generateUnpackedBmp4()
{
	int x,y;
	int i,j;
	int c[2];

	i = 0;
	for (y=0; y<SPRITE4_HEIGHT; ++y) {
		const int yc = y - SPRITE4_HEIGHT/2;
		for (x=0; x<SPRITE4_WIDTH; x+=2) {
			for (j=0; j<2; ++j) {
				const int xc = (x + j) - SPRITE4_WIDTH/2;
				int a = (Atan2F16(xc, yc) + 0x100000) & 0x3FFFFF;
				int r = ((xc*xc + yc*yc) * 48) / (SPRITE4_SIZE / 4);

				if (a < 0x180000) a = 1;
					else a = 0;

				if (r < 1) r = 1;
				if (r > 15) r = 15;

				c[j] = a * r;
			}
			unpackedBmp4[i++] = (c[0] << 4) | c[1];
		}
	}
	pal4[0] = 0;
	setPalGradient(1,15, 31,28,20, 4,8,16, pal4);
}

static void generateUnpackedBmp8()
{
	int x,y,c,i=0;

	for (y=0; y<SPRITE8_HEIGHT; ++y) {
		const int yc = y - SPRITE8_HEIGHT/2;
		for (x=0; x<SPRITE8_WIDTH; ++x) {
			const int xc = x - SPRITE8_WIDTH/2;

			c = xc * xc + yc * yc + ((SinF16(5.0 * Atan2F16(xc, yc)) + 65536) >> 7);
			if (c==0) c = 1;
			c = SPRITE8_SIZE / 4 - c;
			if (c < 0) c = 0;
			c = (32 * c) / (SPRITE8_SIZE / 4);
			c = (c * c) / 3;
			if (c > 31) c = 31;
			unpackedBmp8[i++] = c;
		}
	}
	pal8[0] = 0;
	setPalGradient(1,31, 4,2,6, 31,28,24, pal8);
}

static void generateUnpackedBmp16()
{
	int x,y,i=0;
	const int repX = 63;
	const int repY = 63;

	for (y=0; y<SPRITE16_HEIGHT; ++y) {
		const int yp = (y & repY) - 32;
		for (x=0; x<SPRITE16_WIDTH; ++x) {
			const int xp = (x & repX) - 32;
			int c = xp*xp*xp*xp + yp*yp*yp*yp;
			if (c==0) c = 1;
			c = (48 * 48) - (32767 * 32767) / c;
			if (c < 0) {
				c = 0;
			} else {
				int yy = y;
				if (yy==0) yy = 1;
				c = (((yy >> 3) & 31) << 10) | (((yy >> 2) & 15) << 5) | (yy % 24);
			}
			unpackedBmp16[i++] = c;
		}
	}
}

static void effectInit4()
{
	generateUnpackedBmp4();

	unpackedSpr4 = newSprite(SPRITE4_WIDTH, SPRITE4_HEIGHT, 4, CEL_TYPE_CODED, pal4, unpackedBmp4);
	packedSpr4 = newPackedSprite(SPRITE4_WIDTH, SPRITE4_HEIGHT, 4, CEL_TYPE_CODED, pal4, unpackedBmp4, NULL, 0);
	packedPer4 = packPercentage;
}

static void effectInit8()
{
	generateUnpackedBmp8();

	unpackedSpr8 = newSprite(SPRITE8_WIDTH, SPRITE8_HEIGHT, 8, CEL_TYPE_CODED, pal8, unpackedBmp8);
	packedSpr8 = newPackedSprite(SPRITE8_WIDTH, SPRITE8_HEIGHT, 8, CEL_TYPE_CODED, pal8, unpackedBmp8, NULL, 0);
	packedPer8 = packPercentage;
}

static void effectInit16()
{
	generateUnpackedBmp16();

	unpackedSpr16 = newSprite(SPRITE16_WIDTH, SPRITE16_HEIGHT, 16, CEL_TYPE_UNCODED, NULL, (unsigned char*)unpackedBmp16);
	packedSpr16 = newPackedSprite(SPRITE16_WIDTH, SPRITE16_HEIGHT, 16, CEL_TYPE_UNCODED, NULL, (unsigned char*)unpackedBmp16, NULL, 0);
	packedPer16 = packPercentage;
}

void effectPackedSpritesInit()
{
	initCelPackerEngine();

	loadAndSetBackgroundImage("data/background.img", getBackBuffer());

	if (ENABLE_4) effectInit4();
	if (ENABLE_8) effectInit8();
	if (ENABLE_16) effectInit16();

	deinitCelPackerEngine();
}

static void updateFromInput()
{
	if (ENABLE_4 && isJoyButtonPressedOnce(JOY_BUTTON_A)) showPacked4 = !showPacked4;
	if (ENABLE_8 && isJoyButtonPressedOnce(JOY_BUTTON_B)) showPacked8 = !showPacked8;
	if (ENABLE_16 && isJoyButtonPressedOnce(JOY_BUTTON_C)) showPacked16 = !showPacked16;

	if (isJoyButtonPressedOnce(JOY_BUTTON_LPAD)) {
		if (ENABLE_4) showPacked4 = false;
		if (ENABLE_8) showPacked8 = false;
		if (ENABLE_16) showPacked16 = false;
	}
	if (isJoyButtonPressedOnce(JOY_BUTTON_RPAD)) {
		if (ENABLE_4) showPacked4 = true;
		if (ENABLE_8) showPacked8 = true;
		if (ENABLE_16) showPacked16 = true;
	}

	if (ENABLE_4 && isJoyButtonPressedOnce(JOY_BUTTON_LEFT)) {
		showRender4 = !showRender4;
	}
	if (ENABLE_8 && isJoyButtonPressedOnce(JOY_BUTTON_UP)) {
		showRender8 = !showRender8;
	}
	if (ENABLE_16 && isJoyButtonPressedOnce(JOY_BUTTON_RIGHT)) {
		showRender16 = !showRender16;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_START)) {
		showPackInfo = !showPackInfo;
	}
}

static void renderZoomRotateSprite(Sprite *spr, int offsetX, int offsetY, int zoom, int angle)
{
	setSpritePositionZoomRotate(spr, SCREEN_WIDTH / 2 + offsetX, SCREEN_HEIGHT / 2 + offsetY, zoom, angle);

	drawSprite(spr);
}

static void renderScrollSprite(Sprite *spr, int posX, int posY)
{
	setSpritePosition(spr, posX, posY);

	drawSprite(spr);
}

static void renderText()
{
	char *p4 = " 4bpp: Packed";
	char *p8 = " 8bpp: Packed";
	char *p16 = "16bpp: Packed";
	char *u4 = " 4bpp: Unpacked";
	char *u8 = " 8bpp: Unpacked";
	char *u16 = "16bpp: Unpacked";

	char *t4 = u4;
	char *t8 = u8;
	char *t16 = u16;

	if (showPacked4) t4 = p4;
	if (showPacked8) t8 = p8;
	if (showPacked16) t16 = p16;

	if (showRender4) drawText(192, 8, t4);
	if (showRender8) drawText(192, 16, t8);
	if (showRender16) drawText(192, 24, t16);

	if (showPackInfo) {
		drawNumber(8, 216, packedPer4);
		drawNumber(8, 224, packedPer8);
		drawNumber(8, 232, packedPer16);
	}
}

static void render4(const int time)
{
	const int angle = -time << 8;
	const int px = (SinF16(time * 40000) * 48) >> 16;
	const int py = (SinF16(time * 50000) * 32) >> 16;

	if (showPacked4)
		renderZoomRotateSprite(packedSpr4, px, py, 256, angle);
	else
		renderZoomRotateSprite(unpackedSpr4, px, py, 256, angle);
}

static void render8(const int time)
{
	int i;
	const int angle = time << 8;

	const int numStars = 15;

	for (i=0; i < numStars; ++i)
	{
		const int px = (i & 15) * 20 - 128;
		const int py = ((i * i) & 15) * 12 - 48;
		const int zoom = 128 + (SinF16((time + i*i) << 16) >> 10);

		if (showPacked8)
			renderZoomRotateSprite(packedSpr8, px, py, zoom, angle);
		else
			renderZoomRotateSprite(unpackedSpr8, px, py, zoom, angle);
	}
}

static void render16(const int time)
{
	const int scrollX = -64 + (-time & 63);

	if (showPacked16)
		renderScrollSprite(packedSpr16, scrollX, 0);
	else
		renderScrollSprite(unpackedSpr16, scrollX, 0);
}

static void render(const int time)
{
	if (showRender4) render4(time);
	if (showRender16) render16(time);
	if (showRender8) render8(time);

	renderText();
}

void effectPackedSpritesRun()
{
	updateFromInput();

	render(getFrameNum());
}
