#ifndef PROCGEN_TEXTURE_H
#define PROCGEN_TEXTURE_H

#include "engine_texture.h"

enum {TEXGEN_EMPTY, TEXGEN_FLAT, TEXGEN_NOISE, TEXGEN_XOR, TEXGEN_GRID};

enum {TRI_AREA_LR_TOP, TRI_AREA_LR_BOTTOM, TRI_AREA_RL_TOP, TRI_AREA_RL_BOTTOM};

Texture *initGenTexture(int width, int height, int bpp, uint16 *pal, ubyte numPals, int texgenId, void *params);

void eraseHalfTextureTriangleArea(Texture *tex, int eraseTriangleOrientation);

#endif
