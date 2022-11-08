#include "core.h"

#include "tools.h"
#include "system_graphics.h"

#include "procgen_texture.h"
#include "procgen_image.h"
#include "mathutil.h"


enum {TRI_AREA_LR_TOP, TRI_AREA_LR_BOTTOM, TRI_AREA_RL_TOP, TRI_AREA_RL_BOTTOM};


static void eraseHalfTextureTriangleArea(Texture *tex, int eraseTriangleOrientation, int eraseColor)
{
	int x,y;

	const int width = tex->width;
	const int height = tex->height;

	// very specific works only for 8bpp (32 pal colors) now
	unsigned char *dst = tex->bitmap;
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

	unsigned char *src = tex->bitmap;
	unsigned char *dst = tex->bitmap;
	unsigned char *tempBuff = (unsigned char*)AllocMem(width, MEMTYPE_TRACKSIZE);

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

static void expand8bppTo16bpp(Texture *tex, int rs, int gs, int bs, bool ri, bool gi, bool bi)
{
	const int width = tex->width;
	const int height = tex->height;
	int numPixels = width * height;

	unsigned char *src8 = (unsigned char*)tex->bitmap;
	uint16 *dst16 = (uint16*)tex->bitmap;

	src8 += numPixels-1;
	dst16 += numPixels-1;
	do {
		int r,g,b;
		const unsigned char c = *src8--;

		if (ri) r = c>>rs; else r = (31-c)>>rs;
		if (gi) g = c>>gs; else g = (31-c)>>gs;
		if (bi) b = c>>bs; else b = (31-c)>>bs;

		*dst16-- = MakeRGB15(r,g,b);
	}while(--numPixels > 0);
}

static void genTexture(int texgenId, void *params, Texture *tex)
{
	int i, x, y;

	const int width = tex->width;
	const int height = tex->height;
	const int numPixels = width * height;
	const int size = (numPixels * tex->bpp) / 8;

	ImggenParams paramsDefault = generateImageParamsDefault(width,height,0,31);

	unsigned char *dst = tex->bitmap;
	uint16 *dst16 = (uint16*)dst;

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
				const unsigned char color = *((unsigned char*)params);
				memset(dst, color, size);
			} else {
				const uint16 color = *((uint16*)params);
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
			unsigned char stretch = 0;
			if (params) stretch = *((unsigned char*)params);
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
			generateImage(IMGGEN_GRID, &paramsDefault, dst);
		}
		break;
		
		case TEXGEN_BLOB:
		{
			generateImage(IMGGEN_BLOB, &paramsDefault, dst);
		}
		break;

		case TEXGEN_CLOUDS:
		{
			ImggenParams paramsClouds = generateImageParamsCloud(width,height,1,31, 1,255,127,8,3);

			generateImage(IMGGEN_CLOUDS, &paramsClouds, dst);
			if (tex->bpp==16) {
				expand8bppTo16bpp(tex, 0,2,1, true,true,false);
			}
		}
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

static Texture* initGenTextures(int width, int height, int bpp, uint16 *pal, unsigned char numPals, unsigned char numTextures, int texgenId, bool shade, void *params)
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

Texture* initGenTexture(int width, int height, int bpp, uint16 *pal, unsigned char numPals, int texgenId, void *params)
{
	return initGenTextures(width, height, bpp, pal, numPals, 1, texgenId, false, params);
}

Texture* initGenTextureShades(int width, int height, int bpp, uint16 *pal, unsigned char numPals, int texgenId, int numShades, void *params)
{
	return initGenTextures(width, height, bpp, pal, numPals, numShades, texgenId, true, params);
}

Texture *initGenTexturesTriangleHack(int width, int height, int bpp, uint16 *pal, unsigned char numPals, int texgenId, void *params)
{
	Texture *tex;

	tex = initGenTextures(width, height, bpp, pal, numPals, 2, texgenId, false, params);
	// COMMENT OUT PALETTE BUG
	eraseHalfTextureTriangleArea(&tex[1], TRI_AREA_LR_TOP, 0);

	return tex;
}

Texture *initGenTexturesTriangleHack2(int width, int height, int bpp, uint16 *pal, unsigned char numPals, int texgenId, void *params)
{
	Texture *tex;

	tex = initGenTextures(width, height, bpp, pal, numPals, 2, texgenId, false, params);
	// COMMENT OUT PALETTE BUG
	squishTextureToTriangleArea(&tex[1], TRI_AREA_LR_TOP);

	return tex;
}
