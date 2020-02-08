#ifndef ENGINE_TEXTURE_H
#define ENGINE_TEXTURE_H

extern ushort dracul[];

typedef struct texture
{
    int width, height;
    ubyte *bitmap;
    int bpp;
}texture;

enum {TEXTURE_FLAT, TEXTURE_NOISE, TEXTURE_XOR, TEXTURE_GRID, TEXTURE_DRACUL, TEXTURE_NUM};

void initTexture(int width, int height, int type, int bpp);

texture *getTexture(int textureNum);

#endif
