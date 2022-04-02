#ifndef CEL_PACKER_H
#define CEL_PACKER_H

#include "types.h"
#include "sprite_engine.h"
#include "core.h"

extern int packPercentage;

void initCelPackerEngine(void);
void deinitCelPackerEngine(void);

ubyte* createPackedDataFromUnpackedBmp(int width, int height, int bpp, int type, uint16 *pal, ubyte *unpackedBmp, int transparentColor);

#endif
