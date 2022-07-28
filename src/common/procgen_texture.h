#ifndef PROCGEN_TEXTURE_H
#define PROCGEN_TEXTURE_H

#include "engine_texture.h"

enum {TEXGEN_EMPTY, TEXGEN_FLAT, TEXGEN_NOISE, TEXGEN_XOR, TEXGEN_GRID, TEXGEN_CLOUDS};

Texture *initGenTexture(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, void *params);
Texture *initGenTextureShades(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, int numShades, void *params);
Texture *initGenTexturesTriangleHack(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, void *params);
Texture *initGenTexturesTriangleHack2(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, void *params);

#endif
