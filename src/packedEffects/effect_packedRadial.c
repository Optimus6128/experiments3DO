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

#define FULL_ANGLE 256
#define ANGLE_SKIP 1

static int effectType = EFFECT_ANGLE_TEX;

static Sprite *draculSpr;
static Sprite *unpackedSpr;
static unsigned char *unpackedBmp;

static Sprite **packedSprRadTex;
static Sprite **packedSprAngTex;
static Sprite **packedSprRadCol;
static Sprite **packedSprAngCol;

static unsigned char *angle;
static unsigned char *radius;

static int maxRadius;
static int maxAngle = FULL_ANGLE / ANGLE_SKIP;

static Point2D **posRad;
static Point2D **posAng;
static int *occurrencesRad;
static int *occurrencesAng;


static void initUnpackedSpriteBmp(int a, int b, bool radial)
{
	int i;

	int *occur;
	Point2D **pos;

	unsigned char *src = (unsigned char*)draculSpr->data;
	unsigned char *dst = unpackedBmp;

	const int sprWidth = draculSpr->width;
	const int sprHeight = draculSpr->height;

	memset(unpackedBmp, 0, sprWidth * sprHeight);

	if (radial) {
		occur = occurrencesRad;
		pos = posRad;
	} else {
		occur = occurrencesAng;
		pos = posAng;
	}

	for (i=a; i<=b; ++i) {
		int count = occur[i];
		Point2D *p = pos[i];
		do {
			const int index = p->y * sprWidth + p->x;
			dst[index] = src[index];
			++p;
		} while(--count > 0);
	}
}

/*static void initRadialSpriteBmp(int r)
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
}*/

/*static void initAngleSpriteBmp(int a, int b)
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
}*/

static void initRadialPackedSpritesTex()
{
	int i;
	for (i=0; i<maxRadius; ++i) {
		//initRadialSpriteBmp(i);
		initUnpackedSpriteBmp(i-1, i+1, true);
		packedSprRadTex[i] = newPackedSprite(unpackedSpr->width, unpackedSpr->height, 8, CEL_TYPE_UNCODED, NULL, unpackedBmp, NULL, 0);
		//setSpriteAlpha(packedSprRadTex[i], true, true);
		if (i > 0) linkCel(packedSprRadTex[i-1]->cel, packedSprRadTex[i]->cel);
		
		updateLoadingBar(0, i, maxRadius-1);
	}
}

static void initAnglePackedSpritesTex()
{
	int i,j=0;
	for (i=0; i<FULL_ANGLE; i+=ANGLE_SKIP) {
		//initAngleSpriteBmp(i, i+ANGLE_SKIP);
		initUnpackedSpriteBmp(i-1, i+ANGLE_SKIP+1, false);
		packedSprAngTex[j] = newPackedSprite(unpackedSpr->width, unpackedSpr->height, 8, CEL_TYPE_UNCODED, NULL, unpackedBmp, NULL, 0);
		//setSpriteAlpha(packedSprAngTex[i], true, true);
		if (j > 0) linkCel(packedSprAngTex[j-1]->cel, packedSprAngTex[j]->cel);

		updateLoadingBar(1, j, maxAngle-1);
		++j;
	}
}

static void initRadialPackedSpritesCol()
{
}

static void initAnglePackedSpritesCol()
{
}

static void initRadialSprites()
{
	int x,y,i;

	const int width = draculSpr->width;
	const int height = draculSpr->height;
	const int size = width * height;

	int *radIndex;
	int *angIndex;

	maxRadius = width / 2;
	if (height < width) maxRadius = height / 2;

	packedSprRadTex = (Sprite**)AllocMem(maxRadius * sizeof(Sprite*), MEMTYPE_ANY);
	packedSprAngTex = (Sprite**)AllocMem(maxAngle* sizeof(Sprite*), MEMTYPE_ANY);

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

			if (r > 0 && r < maxRadius) {
				occurrencesRad[r]++;
				occurrencesAng[a]++;
			}

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
			const unsigned char r = radius[i];
			const unsigned char a = angle[i];

			if (r < maxRadius) {
				Point2D *pr = posRad[r];
				Point2D *pa = posAng[a];
				const int ri = radIndex[r];
				const int ai = angIndex[a];

				pr[ri].x = x;
				pr[ri].y = y;
				pa[ai].x = x;
				pa[ai].y = y;
				radIndex[r]++;
				angIndex[a]++;
			}
			++i;
		}
	}

	FreeMem(radIndex, maxRadius * sizeof(int));
	FreeMem(angIndex, maxAngle * sizeof(int));
}

static void animateRadial(int t)
{
	int i;
	for (i=0; i<maxRadius; ++i) {
		const int a = SinF16((i<<16) + (t<<12)) >> 2;
		const int r = 512;//448 + (SinF16((i<<20) + (t<<14)) >> 12);
		setSpritePositionZoomRotate(packedSprRadTex[i], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, r, a);
	}
}

static void animateAngle(int t)
{
	int i,j=0;
	for (i=0; i<FULL_ANGLE; i+=ANGLE_SKIP) {
		const int s = 384 + (SinF16(((ANGLE_SKIP*j)<<19) + (t<<14)) >> 11);
		setSpritePositionZoom(packedSprAngTex[j], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, s);
		++j;
	}
}

static void inputScript()
{
	// For now, I will test effects separately, so no init/switch between all of them in one compile.
	// Because of consideration of not being able to fit all packed CELs for four effects in memory and very long precalc time.
	// So I disable the input script and only init one of four each compile.

	/*if (isJoyButtonPressedOnce(JOY_BUTTON_A)) {
		effectType++;
		if (effectType == EFFECT_TOTAL_NUM) {
			effectType = 0;
		}
	}*/
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

	switch(effectType) {
		case EFFECT_RADIAL_TEX:
			initRadialPackedSpritesTex();
		break;

		case EFFECT_ANGLE_TEX:
			initAnglePackedSpritesTex();
		break;

		case EFFECT_RADIAL_COL:
			initRadialPackedSpritesCol();
		break;

		case EFFECT_ANGLE_COL:
			initAnglePackedSpritesCol();
		break;
	}

	deinitCelPackerEngine();

	FreeMem(occurrencesRad, maxRadius * sizeof(int));
	FreeMem(occurrencesAng, maxAngle * sizeof(int));
}

void effectPackedRadialRun()
{
	const int t = getTicks();

	inputScript();

	switch(effectType) {
		case EFFECT_RADIAL_TEX:
			animateRadial(t);
			drawSprite(packedSprRadTex[0]);
		break;

		case EFFECT_ANGLE_TEX:
			animateAngle(t);
			drawSprite(packedSprAngTex[0]);
		break;

		case EFFECT_RADIAL_COL:
		break;

		case EFFECT_ANGLE_COL:
		break;
	}
}
