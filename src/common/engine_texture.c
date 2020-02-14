#include "core.h"

#include "tools.h"
#include "engine_texture.h"
#include "mathutil.h"

texture *textures[TEXTURE_NUM];


void initTexture(int width, int height, int type, int bpp)
{
	int i, x, y, xc, yc, c;
	int size = (width * height * bpp) / 8;
	texture *tex;

	tex = (texture*)AllocMem(sizeof(texture), MEMTYPE_ANY);
	textures[type] = tex;

	tex->width = width;
	tex->height = height;
	tex->bitmap = (ubyte*)AllocMem(size, MEMTYPE_ANY);
	tex->bpp = bpp;

	switch(type)
	{
		case TEXTURE_EMPTY:
			memset(tex->bitmap, 0, size);
		break;

		case TEXTURE_FLAT:
			for (i=0; i<size; i++)
				tex->bitmap[i] = 31;
		break;

		case TEXTURE_NOISE:
			for (i=0; i<size; i++)
				tex->bitmap[i] = getRand(1, 32767);
		break;

		case TEXTURE_XOR:
			i = 0;
			for (y=0; y<height; y++) {
				for (x=0; x<width; x++) {
					tex->bitmap[i++] = x ^ y;
				}
			}
		break;

		case TEXTURE_GRID:
			i = 0;
			for (y=0; y<height; y++) {
				yc = y - (height >> 1);
				for (x=0; x<width; x++) {
					xc = x - (width >> 1);
					c = (xc * xc * xc * xc + yc * yc * yc * yc) >> 3;
					if (c > 31) c = 31;
					tex->bitmap[i++] = c;
				}
			}
		break;
	}
}

void loadTexture(char *path, int id)
{
	texture *tex;
	CCB *tempCel;
	int size;

	tex = (texture*)AllocMem(sizeof(texture), MEMTYPE_ANY);
	textures[id] = tex;

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
}

texture *getTexture(int textureNum)
{
	return textures[textureNum];
}

/*void initTextures()
{
	initTexture(FB_WIDTH, FB_HEIGHT, TEXTURE_NOISE, 16);
	initTexture(16, 16, TEXTURE_FLAT, 8);
	initTexture(128, 128, TEXTURE_DRACUL, 16);
}*/
