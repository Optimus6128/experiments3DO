#ifndef ENGINE_TEXTURE_H
#define ENGINE_TEXTURE_H

#define TEXTURE_TYPE_DEFAULT		0
#define TEXTURE_TYPE_PALLETIZED		(1 << 0)
#define TEXTURE_TYPE_PACKED			(1 << 1)
#define TEXTURE_TYPE_FEEDBACK		(1 << 2)
#define TEXTURE_TYPE_BLEND			(1 << 3)
#define TEXTURE_TYPE_TWOSIDED		(1 << 4)

typedef struct Texture
{
	int width, height;
	int wShift, hShift;
	int bpp;
	int type;

	// Texture bitmap and palette pointer (needed for 8bpp CODED or less)
	unsigned char *bitmap;
	uint16 *pal;

	// Necessary extension for feedback textures
	int posX, posY;
	int bufferIndex;
}Texture;


void setupTexture(int width, int height, int bpp, int type, unsigned char *bmp, uint16 *pal, unsigned char numPals, Texture *tex);
Texture *initTextures(int width, int height, int bpp, int type, unsigned char *bmp, uint16 *pal, unsigned char numPals, unsigned char numTextures);
Texture *initTexture(int width, int height, int bpp, int type, unsigned char *bmp, uint16 *pal, unsigned char numPals);
Texture *loadTexture(char *path);
Texture *initFeedbackTexture(int posX, int posY, int width, int height, int bufferIndex);

#endif
