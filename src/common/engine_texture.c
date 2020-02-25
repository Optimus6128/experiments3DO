#include "core.h"

#include "tools.h"
#include "system_graphics.h"
#include "engine_texture.h"


Texture* initTexture(int width, int height, int bpp, int type)
{
	const int size = (width * height * bpp) / 8;
	Texture *tex = (Texture*)AllocMem(sizeof(Texture), MEMTYPE_ANY);

	tex->width = width;
	tex->height = height;
	tex->bpp = bpp;
	tex->type = type;

	tex->bitmap = (ubyte*)AllocMem(size, MEMTYPE_ANY);

	return tex;
}

Texture *initFeedbackTexture(int posX, int posY, int width, int height, int bufferIndex)
{
	Texture *tex = (Texture*)AllocMem(sizeof(Texture), MEMTYPE_ANY);

	tex->width = width;
	tex->height = height;
	tex->bpp = 16;
	tex->type = (TEXTURE_TYPE_DYNAMIC | TEXTURE_TYPE_FEEDBACK);


	tex->bitmap = (ubyte*)getBackBufferByIndex(bufferIndex);
	tex->bufferIndex = bufferIndex;
	tex->posX = posX;
	tex->posY = posY;

	return tex;
}

Texture *loadTexture(char *path)
{
	Texture *tex;
	CCB *tempCel;
	int size;

	(Texture*)AllocMem(sizeof(Texture), MEMTYPE_ANY);

	tempCel = LoadCel(path, MEMTYPE_ANY);

	tex = initTexture(tempCel->ccb_Width, tempCel->ccb_Height, 16, TEXTURE_TYPE_STATIC);
		// 16bit is the only bpp CEL type extracted from BMPTo3DOCel for now (which is what I currently use to make CEL files and testing)
		// In the future, I'll try to deduce this from the CEL bits anyway (I already know how, just too lazy to find out again)
		// Update: BMPTo3DOCel is shit! It saves right now the same format as BMPTo3DOImage (for VRAM structure to use with SPORT copy) instead of the most common linear CEL bitmap structure

	size = (tex->width * tex->height * tex->bpp) / 8;

	memcpy(tex->bitmap, tempCel->ccb_SourcePtr, size);

	UnloadCel(tempCel);

	return tex;
}
