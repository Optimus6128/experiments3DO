#include "core.h"

#include "effect_packedRadial.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"
#include "mathutil.h"

#include "sprite_engine.h"
#include "cel_packer.h"


static Sprite *draculSpr;
static Sprite *unpackedSpr;
static unsigned char *unpackedBmp;

static Sprite **packedSpr;

static unsigned char *angle;
static unsigned char *radius;

static int maxRadius;

static void initTestRadius(int r)
{
	int count = draculSpr->width * draculSpr->height;

	unsigned char *rad = radius;
	unsigned char *src = (unsigned char*)draculSpr->data;
	unsigned char *dst = unpackedBmp;

	do {
		if (*rad++ == r) {
			*dst++ = *src;
		} else {
			*dst++ = 0;
		}
		++src;
	}while(--count > 0);
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

void effectPackedRadialInit()
{
	initCelPackerEngine();

	draculSpr = loadSpriteCel("data/draculin.cel");

	unpackedBmp = (unsigned char*)AllocMem(draculSpr->width * draculSpr->height, MEMTYPE_ANY);
	memset(unpackedBmp, 0, draculSpr->width * draculSpr->height);

	unpackedSpr = newSprite(draculSpr->width, draculSpr->height, 8, CEL_TYPE_UNCODED, NULL, unpackedBmp);

	initRadialSprites();
	initTestRadius(16);

	deinitCelPackerEngine();
}

void effectPackedRadialRun()
{
	drawSprite(unpackedSpr);
}
