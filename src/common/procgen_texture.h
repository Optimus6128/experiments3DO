#ifndef PROCGEN_TEXTURE_H
#define PROCGEN_TEXTURE_H

#include "engine_texture.h"

enum {TEXGEN_EMPTY, TEXGEN_FLAT, TEXGEN_NOISE, TEXGEN_XOR, TEXGEN_GRID};

Texture *initGenTexture(int width, int height, int bpp, ubyte numPals, int texgenId, void *params);

#endif
