#include "core.h"

#include "tools.h"
#include "system_graphics.h"
#include "engine_texture.h"
#include "mathutil.h"

static void genTexture(Texture *tex, int texgenId, void *vars)
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
			ubyte color = *((ubyte*)vars);
			memset(dst, color, size);
		}
		break;

		case TEXGEN_NOISE:
		{
			for (i=0; i<size; i++)
				*dst++ = getRand(1, 255);
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
	if (tex->bpp <= 8) tex->type |= TEXTURE_TYPE_PALETIZED;
}

static Texture* initTexture(int width, int height, int bpp)
{
	const int size = (width * height * bpp) / 8;
	Texture *tex = (Texture*)AllocMem(sizeof(Texture), MEMTYPE_ANY);

	tex->type = TEXTURE_TYPE_STATIC;

	tex->width = width;
	tex->height = height;
	tex->bitmap = (ubyte*)AllocMem(size, MEMTYPE_ANY);
	tex->bpp = bpp;

	return tex;
}

Texture* initGenTexture(int width, int height, int bpp, int texgenId, void *vars)
{
	Texture *tex = initTexture(width, height, bpp);
	genTexture(tex, texgenId, vars);
	return tex;
}

Texture *initFeedbackTexture(int posX, int posY, int width, int height, int bufferIndex)
{
	Texture *tex = (Texture*)AllocMem(sizeof(Texture), MEMTYPE_ANY);

	tex->type = (TEXTURE_TYPE_DYNAMIC | TEXTURE_TYPE_FEEDBACK);

	tex->width = width;
	tex->height = height;
	tex->bpp = 16;

	tex->bitmap = (ubyte*)getBackBufferByIndex(bufferIndex);
	tex->posX = posX;
	tex->posY = posY;

	return tex;
}

Texture *loadTexture(char *path)
{
	Texture *tex;
	CCB *tempCel;
	int size;

	tex = (Texture*)AllocMem(sizeof(Texture), MEMTYPE_ANY);

	tempCel = LoadCel(path, MEMTYPE_ANY);

	tex->width = tempCel->ccb_Width;
	tex->height = tempCel->ccb_Height;
	tex->bpp = 16;	// 16bit is the only bpp CEL type extracted from BMPTo3DOCel for now (which is what I currently use to make CEL files and testing)
					// In the future, I'll try to deduce this from the CEL bits anyway (I already know how, just too lazy to find out again)
					// Update: BMPTo3DOCel is shit! It saves right now the same format as BMPTo3DOImage (for VRAM structure to use with SPORT copy) instead of the most common linear CEL bitmap structure
	size = (tex->width * tex->height * tex->bpp) / 8;
	tex->bitmap = (ubyte*)AllocMem(size, MEMTYPE_ANY);

	memcpy(tex->bitmap, tempCel->ccb_SourcePtr, size);

	UnloadCel(tempCel);

	return tex;
}
