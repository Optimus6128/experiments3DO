#ifndef CEL_PACKER_H
#define CEL_PACKER_H

#include "types.h"
#include "sprite_engine.h"
#include "main_includes.h"

extern int packPercentage;

ubyte* createPackedDataFromUnpackedBmp(int width, int height, int bpp, int type, uint16 *pal, ubyte *unpackedBmp);

#endif
