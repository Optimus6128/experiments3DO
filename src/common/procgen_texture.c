#include "core.h"

#include "tools.h"
#include "system_graphics.h"

#include "procgen_texture.h"
#include "mathutil.h"

static void genTexture(Texture *tex, int texgenId, void *params)
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
					*dst++ = x ^ y;
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

Texture* initGenTexture(int width, int height, int bpp, ubyte numPals, int texgenId, void *params)
{
	Texture *tex;

	int type = TEXTURE_TYPE_STATIC;
	if (bpp <= 8 && numPals > 0) type |= TEXTURE_TYPE_PALLETIZED;

	tex = initTexture(width, height, bpp, type, NULL, NULL, numPals);
	genTexture(tex, texgenId, params);

	return tex;
}
