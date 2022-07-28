#include "core.h"

#include "system_graphics.h"
#include "engine_texture.h"
#include "cel_helpers.h"

#include "mathutil.h"

void setupTexture(int width, int height, int bpp, int type, ubyte *bmp, uint16 *pal, ubyte numPals, Texture *tex)
{
	// Can't have palettized texture if bpp over 8
	if (bpp > 8 || numPals == 0) {
		type &= ~TEXTURE_TYPE_PALLETIZED;
	}

	tex->width = width;
	tex->height = height;
	tex->wShift = getShr(width);
	tex->hShift = getShr(height);
	tex->bpp = bpp;
	tex->type = type;

	if (type & TEXTURE_TYPE_PALLETIZED) {
		if (!pal) {
			int palBits = bpp;
			if (palBits > 5) palBits = 5;	// Can't have more than 5bits (32 colors palette), even in 6bpp or 8bpp modes
			tex->pal = (uint16*)AllocMem(sizeof(uint16) * (1 << palBits) * numPals, MEMTYPE_ANY);
		} else {
			tex->pal = pal;
		}
	}

	if (!bmp) {
		const int size = (width * height * bpp) / 8;
		tex->bitmap = (ubyte*)AllocMem(size, MEMTYPE_ANY);
	} else {
		tex->bitmap = bmp;
	}
}

Texture* initTextures(int width, int height, int bpp, int type, ubyte *bmp, uint16 *pal, ubyte numPals, ubyte numTextures)
{
	int i;
	Texture *texs = (Texture*)AllocMem(numTextures * sizeof(Texture), MEMTYPE_ANY);

	for (i=0; i<numTextures; ++i) {
		setupTexture(width, height, bpp, type, bmp, pal, numPals, &texs[i]);
	}

	return texs;
}

Texture *initTexture(int width, int height, int bpp, int type, ubyte *bmp, uint16 *pal, ubyte numPals)
{
	return initTextures(width, height, bpp, type, bmp, pal, numPals, 1);
}

Texture *initFeedbackTexture(int posX, int posY, int width, int height, int bufferIndex)
{
	Texture *tex = (Texture*)AllocMem(sizeof(Texture), MEMTYPE_ANY);

	tex->width = width;
	tex->height = height;
	tex->bpp = 16;
	tex->type = TEXTURE_TYPE_FEEDBACK;


	tex->bitmap = (ubyte*)getBackBufferByIndex(bufferIndex);
	tex->bufferIndex = bufferIndex;
	tex->posX = posX;
	tex->posY = posY;

	return tex;
}

Texture *loadTexture(char *path)
{
	CCB *tempCel = LoadCel(path, MEMTYPE_ANY);

	Texture *tex = initTexture(tempCel->ccb_Width, tempCel->ccb_Height, getCelBpp(tempCel), TEXTURE_TYPE_DEFAULT, NULL, NULL, 0);
		// 16bit is the only bpp CEL type extracted from BMPTo3DOCel for now (which is what I currently use to make CEL files and testing)
		// In the future, I'll try to deduce this from the CEL bits anyway (I already know how, just too lazy to find out again)
		// Update: BMPTo3DOCel is shit! It saves right now the same format as BMPTo3DOImage (for VRAM structure to use with SPORT copy) instead of the most common linear CEL bitmap structure

	memcpy(tex->bitmap, tempCel->ccb_SourcePtr, getCelDataSizeInBytes(tempCel));

	UnloadCel(tempCel);

	return tex;
}
