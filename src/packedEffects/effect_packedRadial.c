#include "core.h"

#include "effect_packedRadial.h"

#include "mathutil.h"
#include "system_graphics.h"
#include "tools.h"
#include "input.h"

#include "sprite_engine.h"
#include "cel_helpers.h"
#include "cel_packer.h"


static Sprite *draculSpr;
static Sprite *unpackedSpr;
static unsigned char *unpackedBmp;

static Sprite **packedSpr;

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
		if (rr==r || rr==r+1) {	// one rad more to overlap and avoid gaps
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
		packedSpr[i] = newPackedSprite(unpackedSpr->width, unpackedSpr->height, 8, CEL_TYPE_UNCODED, NULL, unpackedBmp, NULL, 0);
		if (i > 0) linkCel(packedSpr[i-1]->cel, packedSpr[i]->cel);
	}
}

static void initRadialSprites()
{
	int x,y,i;

	const int width = draculSpr->width;
	const int height = draculSpr->width;

	maxRadius = width / 2;
	if (height < width) maxRadius = height / 2;

	packedSpr = (Sprite**)AllocMem(maxRadius * sizeof(Sprite*), MEMTYPE_ANY);

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

static void animateRadial()
{
	int i;
	const int t = getTicks();

	for (i=0; i<maxRadius; ++i) {
		const int a = SinF16((i<<16) + (t<<12)) >> 2;
		const int r = 512;//448 + (SinF16((i<<20) + (t<<14)) >> 12);
		setSpritePositionZoomRotate(packedSpr[i], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, r, a);
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

	deinitCelPackerEngine();
}

void effectPackedRadialRun()
{
	animateRadial();

	drawSprite(packedSpr[0]);
}
