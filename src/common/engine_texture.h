#ifndef ENGINE_TEXTURE_H
#define ENGINE_TEXTURE_H

#define TEXTURE_TYPE_STATIC 0
#define TEXTURE_TYPE_DYNAMIC 1
#define TEXTURE_TYPE_FEEDBACK 2
#define TEXTURE_TYPE_PALETIZED 4
#define TEXTURE_TYPE_BLEND 8
#define TEXTURE_TYPE_TWOSIDED 16

typedef struct Texture
{
	int type;
	int width, height;
	int bpp;

	// Texture bitmap and palette pointer (needed for 8bpp CODED or less)
	ubyte *bitmap;
	ushort *pal;

	// Necessary extension for feedback textures
	int posX, posY;
	int bufferIndex;
}Texture;

enum {TEXGEN_EMPTY, TEXGEN_FLAT, TEXGEN_NOISE, TEXGEN_XOR, TEXGEN_GRID};

Texture *loadTexture(char *path);
Texture *initFeedbackTexture(int posX, int posY, int width, int height, int bufferIndex);
Texture *initGenTexture(int width, int height, int bpp, int texgenId, void *vars);

#endif
