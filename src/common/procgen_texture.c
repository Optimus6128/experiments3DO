#include "core.h"

#include "tools.h"
#include "system_graphics.h"

#include "procgen_texture.h"
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


static void eraseHalfTextureTriangleArea(Texture *tex, int eraseTriangleOrientation, int eraseColor)
{
	int x,y;

	const int width = tex->width;
	const int height = tex->height;

	// very specific works only for 8bpp (32 pal colors) now
	ubyte *dst = tex->bitmap;
	for (y=0; y<height; ++y) {
		for (x=0; x<width; ++x) {
			bool erase = false;
			switch(eraseTriangleOrientation)
			{
				case TRI_AREA_LR_TOP:
					erase = (width - x < y);
				break;

				case TRI_AREA_LR_BOTTOM:
					erase = (width - x > y);
				break;

				case TRI_AREA_RL_TOP:
					erase = (x < y);
				break;

				case TRI_AREA_RL_BOTTOM:
					erase = (x > y);
				break;
			}
			if (erase) {
				*dst = eraseColor;
			} else {
				if (*dst==eraseColor) *dst = (eraseColor + 1) & 31;
			}
			dst++;
		}
	}
	// COMMENT OUT PALETTE BUG
	tex->pal[32 + eraseColor] = 0;
}

void squishTextureToTriangleArea(Texture *tex, int eraseTriangleOrientation)
{
	int x,y;

	const int width = tex->width;
	const int height = tex->height;

	ubyte *src = tex->bitmap;
	ubyte *dst = tex->bitmap;
	ubyte *tempBuff = (ubyte*)AllocMem(width, MEMTYPE_TRACKSIZE);

	// I will do one triangle orientation for now
	for (y=0; y<height; ++y) {
		memcpy(tempBuff, src, width);
		for (x=0; x<width; ++x) {
			const int xp = (x * (height - y)) / height;
			*dst++ = *(tempBuff + xp);
		}
		src += width;
	}

	// COMMENT OUT PALETTE BUG
	FreeMem(tempBuff, -1);
}

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

static int addLightBlobToTexel(int x, int y, int width, int height)
{
	int xc = x - width / 2;
	int yc = y - height / 2;

	int r = xc * xc + yc * yc;
	if (r == 0) r = 1;
	return 256 / isqrt(r);
}

static void genCloudTexture(int hashX, int hashY, int hashZ, int shrStart, int iterations, Texture *tex)
{
	int x, y;
	uint8 *dst8 = (uint8*)tex->bitmap;
	uint16 *dst16 = (uint16*)tex->bitmap;
	const int width = tex->width;
	const int height = tex->height;

	hx = hashX;
	hy = hashY;
	hz = hashZ;

	shift = shrStart;
	iters = iterations;

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			int c = perlinOctave(x, y) >> RANGE_TO_COL;
			int a = addLightBlobToTexel(x, y, width, height);
			if (tex->bpp==8) {
				//c += a;
				//CLAMP(c, 1, 31)
				*dst8++ = c;
			} else {
				int cr = 31-c+a;
				int cg = c+a;
				int cb = c+a;
				CLAMP(cr, 1, 31)
				CLAMP(cg, 1, 63)
				CLAMP(cb, 1, 31)
				*dst16++ = ((cr>>0) << 10) | ((cg>>1) << 5) | cb;
			}
		}
	}
}

static void genTexture(int texgenId, void *params, Texture *tex)
{
	int i, x, y, xc, yc, c;

	const int width = tex->width;
	const int height = tex->height;
	const int size = (width * height * tex->bpp) / 8;

	ubyte *dst = tex->bitmap;

	if (!genRandValues) {
		for (i = 0; i < RANDV_NUM; ++i)
			randV[i] = (SinF16(getRand(0, 32767) * getRand(0, 32767)) * ((RANGE >> 1) - 1)) >> 16;
		genRandValues = true;
	}

	// Right now, these are suitable for 8bpp with 32 color palette, no check is happening whether the texture is in other bpp modes
	switch(texgenId)
	{
		default:
		case TEXGEN_EMPTY:
		{
			memset(dst, 0, size);
		}
		break;

		case TEXGEN_FLAT:
		{
			if (tex->bpp<=8) {
				const ubyte color = *((ubyte*)params);
				memset(dst, color, size);
			} else {
				const uint16 color = *((uint16*)params);
				uint16 *dst16 = (uint16*)dst;
				for (i=0; i<size/2; ++i) {
					*dst16++ = color;
				}
			}
		}
		break;

		case TEXGEN_NOISE:
		{
			for (i=0; i<size; i++)
				*dst++ = getRand(0, 31);
		}
		break;

		case TEXGEN_XOR:
		{
			ubyte stretch = 0;
			if (params) stretch = *((ubyte*)params);
			for (y=0; y<height; y++) {
				for (x=0; x<width; x++) {
					if (stretch==0) {
						*dst++ = (x ^ y) & 31;
					} else {
						*dst++ = (x ^ ((y*stretch) & 15)) & 31;
					}
				}
			}
		}
		break;

		case TEXGEN_GRID:
		{
			for (y=0; y<height; y++) {
				yc = y - (height >> 1);
				for (x=0; x<width; x++) {
					xc = x - (width >> 1);
					c = (xc * xc * xc * xc + yc * yc * yc * yc) >> 8;
					if (c > 31) c = 31;
					*dst++ = c;
				}
			}
		}
		break;

		case TEXGEN_CLOUDS:
			genCloudTexture(1,255,127, 8,3, tex);
		break;
	}
}

static void copyTextureData(Texture *src, Texture *dst)
{
	const int size = (src->width * src->height * src->bpp) >> 3;
	memcpy(dst->bitmap, src->bitmap, size);
}

static void copyAndShadeTextureData(Texture *src, Texture *dst, int shade, int bright)
{
	const int width = src->width;
	const int height = src->height;
	const int size = (width * height * src->bpp) >> 3;
	const int size16 = size / 2;

	int fpShade = (shade << FP_CORE) / bright;

	switch(src->bpp) {
		case 8:
		{
			// no point as we can use palette, unless it's 8bpp without palette, but now we wrote this function the for 16bpp gouraud shaded envmap.
		}
		break;

		case 16:
		{
			int i;
			uint16 *srcData = (uint16*)src->bitmap;
			uint16 *dstData = (uint16*)dst->bitmap;

			for (i=0; i<size16; ++i) {
				uint16 c = *srcData++;
				const int r = (((c >> 10) & 31) * fpShade) >> FP_CORE;
				const int g = (((c >> 5) & 31) * fpShade) >> FP_CORE;
				const int b = ((c  & 31) * fpShade) >> FP_CORE;
				*dstData++ = (r << 10) | (g << 5) | b;
			}
		}
		break;

		default:
			// won't take care or might not need of lesser bpp as you can use the palette to shade instead.
		break;
	}
}

static Texture* initGenTextures(int width, int height, int bpp, uint16 *pal, ubyte numPals, ubyte numTextures, int texgenId, bool shade, void *params)
{
	int i;
	Texture *tex;

	int type = TEXTURE_TYPE_DEFAULT;
	if (bpp <= 8 && numPals > 0) type |= TEXTURE_TYPE_PALLETIZED;

	tex = initTextures(width, height, bpp, type, NULL, pal, numPals, numTextures);
	genTexture(texgenId, params, &tex[0]);

	for (i=1; i<numTextures; ++i) {
		if (!shade) {
			copyTextureData(&tex[0],&tex[i]);
		} else {
			copyAndShadeTextureData(&tex[0],&tex[i], numTextures-i, numTextures);
		}
	}

	return tex;
}

Texture* initGenTexture(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, void *params)
{
	return initGenTextures(width, height, bpp, pal, numPals, 1, texgenId, false, params);
}

Texture* initGenTextureShades(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, int numShades, void *params)
{
	return initGenTextures(width, height, bpp, pal, numPals, numShades, texgenId, true, params);
}

Texture *initGenTexturesTriangleHack(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, void *params)
{
	Texture *tex;

	tex = initGenTextures(width, height, bpp, pal, numPals, 2, texgenId, false, params);
	// COMMENT OUT PALETTE BUG
	eraseHalfTextureTriangleArea(&tex[1], TRI_AREA_LR_TOP, 0);

	return tex;
}

Texture *initGenTexturesTriangleHack2(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, void *params)
{
	Texture *tex;

	tex = initGenTextures(width, height, bpp, pal, numPals, 2, texgenId, false, params);
	// COMMENT OUT PALETTE BUG
	squishTextureToTriangleArea(&tex[1], TRI_AREA_LR_TOP);

	return tex;
}
