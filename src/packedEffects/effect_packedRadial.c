#include "core.h"

#include "effect_packedRadial.h"

#include "mathutil.h"
#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "sprite_engine.h"
#include "cel_helpers.h"
#include "cel_packer.h"

enum { EFFECT_RADIAL_TEX, EFFECT_ANGLE_TEX, EFFECT_RADIAL_COL, EFFECT_ANGLE_COL, EFFECT_TOTAL_NUM };

#define MAX_ANGLE 256
#define ANGLE_SKIP 1

static int effectType = EFFECT_RADIAL_TEX;

static Sprite *draculSpr;
static Sprite *unpackedSpr;
static unsigned char *unpackedBmp;

static Sprite **packedSprRad;
static Sprite **packedSprAng;

static unsigned char *angle;
static unsigned char *radius;

static int maxRadius;



static void initRadialSpriteBmp(int r)
{
	int count = draculSpr->width * draculSpr->height;

	unsigned char *rad = radius;
	unsigned char *src = (unsigned char*)draculSpr->data;
	unsigned char *dst = unpackedBmp;

	do {
		unsigned char rr = *rad++;
		if (rr>=r-1 && rr<=r+1) {	// one rad more to overlap and avoid gaps
			*dst++ = *src;
		} else {
			*dst++ = 0;
		}
		++src;
	}while(--count > 0);
}

static void initAngleSpriteBmp(int a, int b)
{
	int count = draculSpr->width * draculSpr->height;

	unsigned char *ang = angle;
	unsigned char *src = (unsigned char*)draculSpr->data;
	unsigned char *dst = unpackedBmp;

	do {
		unsigned char aa = *ang++;
		if (aa>=a-1 && aa<=b+1) {
			*dst++ = *src;
		} else {
			*dst++ = 0;
		}
		++src;
	}while(--count > 0);
}

static void initRadialPackedSprites()
{
	int i;
	for (i=0; i<maxRadius; ++i) {
		initRadialSpriteBmp(i);
		packedSprRad[i] = newPackedSprite(unpackedSpr->width, unpackedSpr->height, 8, CEL_TYPE_UNCODED, NULL, unpackedBmp, NULL, 0);
		//setSpriteAlpha(packedSprRad[i], true, true);
		if (i > 0) linkCel(packedSprRad[i-1]->cel, packedSprRad[i]->cel);
	}
}

static void initAnglePackedSprites()
{
	int i,j=0;
	for (i=0; i<MAX_ANGLE; i+=ANGLE_SKIP) {
		initAngleSpriteBmp(i, i+ANGLE_SKIP);
		packedSprAng[j] = newPackedSprite(unpackedSpr->width, unpackedSpr->height, 8, CEL_TYPE_UNCODED, NULL, unpackedBmp, NULL, 0);
		//setSpriteAlpha(packedSprAng[i], true, true);
		if (j > 0) linkCel(packedSprAng[j-1]->cel, packedSprAng[j]->cel);
		++j;
	}
}

static void initRadialSprites()
{
	int x,y,i;

	const int width = draculSpr->width;
	const int height = draculSpr->width;

	maxRadius = width / 2;
	if (height < width) maxRadius = height / 2;

	packedSprRad = (Sprite**)AllocMem(maxRadius * sizeof(Sprite*), MEMTYPE_ANY);
	packedSprAng = (Sprite**)AllocMem((MAX_ANGLE / ANGLE_SKIP)* sizeof(Sprite*), MEMTYPE_ANY);

	radius = (unsigned char*)AllocMem(width * height, MEMTYPE_ANY);
	angle = (unsigned char*)AllocMem(width * height, MEMTYPE_ANY);

	i = 0;
	for (y=0; y<height; ++y) {
		const int yc = y - height / 2;
		for (x=0; x<width; ++x) {
			const int xc = x - width / 2;

			const int r = isqrt(xc * xc + yc * yc);
			const unsigned char a = (unsigned char)(Atan2F16(xc,yc) >> 16);

			angle[i] = a;
			radius[i] = r;
			++i;
		}
	}
}

static void animateRadial(int t)
{
	int i;
	for (i=0; i<maxRadius; ++i) {
		const int a = SinF16((i<<16) + (t<<12)) >> 2;
		const int r = 512;//448 + (SinF16((i<<20) + (t<<14)) >> 12);
		setSpritePositionZoomRotate(packedSprRad[i], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, r, a);
	}
}

static void animateAngle(int t)
{
	int i,j=0;
	for (i=0; i<MAX_ANGLE; i+=ANGLE_SKIP) {
		const int s = 384 + (SinF16(((ANGLE_SKIP*j)<<19) + (t<<14)) >> 11);
		setSpritePositionZoom(packedSprAng[j], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, s);
		++j;
	}
}

static void inputScript()
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_A)) {
		effectType++;
		if (effectType == EFFECT_TOTAL_NUM) {
			effectType = 0;
		}
	}
}

void effectPackedRadialInit()
{
	initCelPackerEngine();

	draculSpr = loadSpriteCel("data/draculin.cel");

	unpackedBmp = (unsigned char*)AllocMem(draculSpr->width * draculSpr->height, MEMTYPE_ANY);
	memset(unpackedBmp, 0, draculSpr->width * draculSpr->height);

	unpackedSpr = newSprite(draculSpr->width, draculSpr->height, 8, CEL_TYPE_UNCODED, NULL, unpackedBmp);

	initRadialSprites();
	initRadialPackedSprites();
	initAnglePackedSprites();

	deinitCelPackerEngine();
}

void effectPackedRadialRun()
{
	const int t = getTicks();

	inputScript();

	switch(effectType) {
		case EFFECT_RADIAL_TEX:
		{
			animateRadial(t);
			drawSprite(packedSprRad[0]);
		}
		break;

		case EFFECT_ANGLE_TEX:
		{
			animateAngle(t);
			drawSprite(packedSprAng[0]);
		}
		break;

		case EFFECT_RADIAL_COL:
		{
		}
		break;

		case EFFECT_ANGLE_COL:
		{
		}
		break;
	}
}
