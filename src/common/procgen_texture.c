#include "core.h"

#include "tools.h"
#include "system_graphics.h"

#include "procgen_texture.h"
#include "mathutil.h"


enum {TRI_AREA_LR_TOP, TRI_AREA_LR_BOTTOM, TRI_AREA_RL_TOP, TRI_AREA_RL_BOTTOM};

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
	tex->pal[32 + eraseColor] = 0;
}

static void genTexture(int texgenId, void *params, Texture *tex)
{
	int i, x, y, xc, yc, c;

	const int width = tex->width;
	const int height = tex->height;
	const int size = (width * height * tex->bpp) / 8;

	ubyte *dst = tex->bitmap;

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
			const ubyte color = *((ubyte*)params);
			memset(dst, color, size);
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
			for (y=0; y<height; y++) {
				for (x=0; x<width; x++) {
					*dst++ = (x ^ y) & 31;
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
					c = (xc * xc * xc * xc + yc * yc * yc * yc) >> 3;
					if (c > 31) c = 31;
					*dst++ = c;
				}
			}
		}
		break;
	}
}

static Texture* initGenTextures(int width, int height, int bpp, uint16 *pal, ubyte numPals, ubyte numTextures, int texgenId, void *params)
{
	int i;
	Texture *tex;

	int type = TEXTURE_TYPE_STATIC;
	if (bpp <= 8 && numPals > 0) type |= TEXTURE_TYPE_PALLETIZED;

	tex = initTextures(width, height, bpp, type, NULL, pal, numPals, numTextures);
	for (i=0; i<numTextures; ++i) {
		genTexture(texgenId, params, &tex[i]);
	}

	return tex;
}

Texture* initGenTexture(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, void *params)
{
	return initGenTextures(width, height, bpp, pal, numPals, 1, texgenId, params);
}

Texture *initGenTexturesTriangleHack(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, void *params)
{
	Texture *tex;

	tex = initGenTextures(width, height, bpp, pal, numPals, 2, texgenId, params);
	eraseHalfTextureTriangleArea(&tex[1], TRI_AREA_LR_TOP, 0);

	return tex;
}
