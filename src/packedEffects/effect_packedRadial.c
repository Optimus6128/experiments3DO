#include "core.h"

#include "effect_packedRadial.h"

#include "mathutil.h"
#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "sprite_engine.h"
#include "cel_helpers.h"
#include "cel_packer.h"

//48

enum { EFFECT_RADIAL_TEX, EFFECT_ANGLE_TEX, EFFECT_RADIAL_COL, EFFECT_ANGLE_COL, EFFECT_TOTAL_NUM };

#define FULL_ANGLE 256
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
static int maxAngle = FULL_ANGLE / ANGLE_SKIP;

static Point2D **posRad;
static Point2D **posAng;

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
	for (i=0; i<FULL_ANGLE; i+=ANGLE_SKIP) {
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
	const int height = draculSpr->height;
	const int size = width * height;

	int *occurrencesRad;
	int *occurrencesAng;
	int *radIndex;
	int *angIndex;

	maxRadius = width / 2;
	if (height < width) maxRadius = height / 2;

	packedSprRad = (Sprite**)AllocMem(maxRadius * sizeof(Sprite*), MEMTYPE_ANY);
	packedSprAng = (Sprite**)AllocMem(maxAngle* sizeof(Sprite*), MEMTYPE_ANY);

	occurrencesRad = (int*)AllocMem(maxRadius * sizeof(int), MEMTYPE_ANY);
	occurrencesAng = (int*)AllocMem(maxAngle * sizeof(int), MEMTYPE_ANY);
	radIndex = (int*)AllocMem(maxRadius * sizeof(int), MEMTYPE_ANY);
	angIndex = (int*)AllocMem(maxAngle * sizeof(int), MEMTYPE_ANY);

	radius = (unsigned char*)AllocMem(size, MEMTYPE_ANY);
	angle = (unsigned char*)AllocMem(size, MEMTYPE_ANY);

	memset(occurrencesRad, 0, maxRadius * sizeof(int));
	memset(occurrencesAng, 0, maxAngle * sizeof(int));
	memset(radIndex, 0, maxRadius * sizeof(int));
	memset(angIndex, 0, maxAngle * sizeof(int));

	i = 0;
	for (y=0; y<height; ++y) {
		const int yc = y - height / 2;
		for (x=0; x<width; ++x) {
			const int xc = x - width / 2;

			const int r = isqrt(xc * xc + yc * yc);
			const unsigned char a = (unsigned char)(Atan2F16(xc,yc) >> 16);

			if (r < maxRadius) {
				++occurrencesRad[r];
			}
			++occurrencesAng[a];

			angle[i] = a;
			radius[i] = r;
			++i;
		}
	}


	posRad = (Point2D**)AllocMem(maxRadius * sizeof(Point2D*), MEMTYPE_ANY);
	for (i=0; i<maxRadius; ++i) {
		posRad[i] = (Point2D*)AllocMem(occurrencesRad[i] * sizeof(Point2D), MEMTYPE_ANY);
	}

	posAng = (Point2D**)AllocMem(maxAngle * sizeof(Point2D*), MEMTYPE_ANY);
	for (i=0; i<maxAngle; ++i) {
		posAng[i] = (Point2D*)AllocMem(occurrencesAng[i] * sizeof(Point2D), MEMTYPE_ANY);
	}


	for (y=0; y<height; ++y) {
		for (x=0; x<width; ++x) {
			unsigned char r = radius[i];
			unsigned char a = angle[i];

			if (r < maxRadius) {
				Point2D *pr = posRad[r];
				Point2D *pa = posAng[a];
				const int ri = radIndex[r];
				const int ai = angIndex[a];

				pr[ri].x = x;
				pr[ri].y = y;
				pa[ai].x = x;
				pa[ai].y = y;
				++radIndex[r];
				++angIndex[a];
			}
		}
	}

	FreeMem(occurrencesRad, maxRadius * sizeof(int));
	FreeMem(occurrencesAng, maxRadius * sizeof(int));
	FreeMem(radIndex, maxRadius * sizeof(int));
	FreeMem(angIndex, maxRadius * sizeof(int));
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
	for (i=0; i<FULL_ANGLE; i+=ANGLE_SKIP) {
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
	int size;

	initCelPackerEngine();

	draculSpr = loadSpriteCel("data/draculin.cel");

	size = draculSpr->width * draculSpr->height;
	unpackedBmp = (unsigned char*)AllocMem(size, MEMTYPE_ANY);
	memset(unpackedBmp, 0, size);

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
