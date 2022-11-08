#include "core.h"

#include "tools.h"
#include "system_graphics.h"

#include "procgen_image.h"
#include "mathutil.h"


#define RANGE_SHR 12
#define RANGE 4096
#define RANGE_COL 5
#define RANGE_TO_COL (RANGE_SHR - RANGE_COL)

#define FIX_MUL(a,b) ((a * b) >> RANGE_SHR)
#define RANDV_NUM 256

static bool genRandValues = false;


enum {TRI_AREA_LR_TOP, TRI_AREA_LR_BOTTOM, TRI_AREA_RL_TOP, TRI_AREA_RL_BOTTOM};

static int hx, hy, hz;
static int shift, iters;

static int randV[RANDV_NUM];

static int dotGradDist(int x, int y, int sx, int sy)
{
	const unsigned int randVindex = (((x >> RANGE_SHR) + hx) * ((y >> RANGE_SHR) + hy) * ((x >> RANGE_SHR) * (y >> RANGE_SHR) + hz)) & (RANDV_NUM - 1);
	const int vx = randV[randVindex];
	const int vy = randV[(randVindex + 1) & (RANDV_NUM - 1)];

	return (vx * (sx - x) + vy * (sy - y)) >> RANGE_SHR;
}

static int smooth(int t) {
	return FIX_MUL(FIX_MUL(FIX_MUL(t, t), t), (FIX_MUL(t, (FIX_MUL(t, (6 << RANGE_SHR)) - (15 << RANGE_SHR))) + (10 << RANGE_SHR)));         // 6t^5 - 15t^4 + 10t^3
}

static int lerp(int n0, int n1, int t)
{
	t = smooth(t);
	return n0 * (RANGE - t - 1) + n1 * t;
}

static int perlin(int x, int y)
{
	const int px = x & (RANGE - 1);
	const int py = y & (RANGE - 1);

	const int x0 = x - px;
	const int y0 = y - py;
	const int x1 = x0 + RANGE;
	const int y1 = y0 + RANGE;

	const int sx = x0 + px;
	const int sy = y0 + py;

	const int n00 = dotGradDist(x0, y0, sx, sy);
	const int n10 = dotGradDist(x1, y0, sx, sy);
	const int n01 = dotGradDist(x0, y1, sx, sy);
	const int n11 = dotGradDist(x1, y1, sx, sy);

	const int ix0 = lerp(n00, n10, px) >> RANGE_SHR;
	const int ix1 = lerp(n01, n11, px) >> RANGE_SHR;

	int c = lerp(ix0, ix1, py) >> RANGE_SHR;

	return c;
}

static int perlinOctave(int x, int y)
{
	int i;
	int c = 0;
	int d = 0;
	int s = shift;
	for (i = 0; i < iters; ++i) {
		c += perlin(x << s, y << s) >> d;
		++d;
		++s;
	}

	c += (RANGE >> 1);
	if (c < 0) c = 0;
	if (c > RANGE - 1) c = RANGE - 1;
	return c;
}

static void genCloudTexture(int width, int height, int rangeMin, int rangeMax, ImggenParams *params, unsigned char *imgData)
{
	int x, y;

	hx = params->hashX;
	hy = params->hashY;
	hz = params->hashZ;

	shift = params->shrStart;
	iters = params->iterations;

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			int c = perlinOctave(x, y) >> RANGE_TO_COL;
			CLAMP(c, rangeMin, rangeMax)
			*imgData++ = c;
		}
	}
}

static void genGridTexture(int width, int height, int rangeMin, int rangeMax, unsigned char *imgData)
{
	int x, y;
	int xc, yc;
	int c;

	for (y=0; y<height; y++) {
		yc = y - (height >> 1);
		for (x=0; x<width; x++) {
			xc = x - (width >> 1);
			c = (xc * xc * xc * xc + yc * yc * yc * yc) >> 8;
			if (c > 31) c = 31;
			*imgData++ = c;
		}
	}
}

static void genBlobTexture(int width, int height, int rangeMin, int rangeMax, unsigned char *imgData)
{
	int x, y;
	int xc, yc;
	int c;

	for (y=0; y<height; y++) {
		yc = y - (height >> 1);
		for (x=0; x<width; x++) {
			xc = x - (width >> 1);
			c = (xc * xc + yc * yc) << 1;
			if (c > 31) c = 31;
			*imgData++ = 31 - c;
		}
	}
}

void generateImage(int imggenId, ImggenParams *params, unsigned char *imgPtr)
{
	const int width = params->width;
	const int height = params->height;
	const int rangeMin = params->rangeMin;
	const int rangeMax = params->rangeMax;

	int i;
	if (!genRandValues) {
		for (i = 0; i < RANDV_NUM; ++i)
			randV[i] = (SinF16(getRand(0, 32767) * getRand(0, 32767)) * ((RANGE >> 1) - 1)) >> 16;
		genRandValues = true;
	}

	switch(imggenId) {
		case IMGGEN_BLOB:
		{
			genBlobTexture(width, height, rangeMin, rangeMax, imgPtr);

		}
		break;

		case IMGGEN_GRID:
		{
			genGridTexture(width, height, rangeMin, rangeMax, imgPtr);

		}
		break;

		case IMGGEN_CLOUDS:
		{
			genCloudTexture(width, height, rangeMin, rangeMax, params, imgPtr);
		}
		break;
		
		default:
		break;
	}
}


ImggenParams generateImageParamsDefault(int width, int height, int rangeMin, int rangeMax)
{
	ImggenParams params;

	params.width = width;
	params.height = height;
	params.rangeMin = rangeMin;
	params.rangeMax = rangeMax;

	return params;
}

ImggenParams generateImageParamsCloud(int width, int height, int rangeMin, int rangeMax, int hashX, int hashY, int hashZ, int shrStart, int iterations)
{
	ImggenParams params;

	params.width = width;
	params.height = height;
	params.rangeMin = rangeMin;
	params.rangeMax = rangeMax;

	params.hashX = hashX;
	params.hashY = hashY;
	params.hashZ = hashZ;
	params.shrStart = shrStart;
	params.iterations = iterations;

	return params;
}
