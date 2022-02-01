#ifndef CEL_HELPERS_H
#define CEL_HELPERS_H

#include "types.h"
#include "core.h"

enum {
	CEL_TYPE_CODED = 0,
	CEL_TYPE_UNCODED = 1,
	CEL_TYPE_PACKED = 2	// Should continue as power of two from here on. You could have (CEL_TYPE_CODED | CEL_TYPE_PACKED) as argument combined.
};

#define CEL_BLEND_OPAQUE 0x1F001F00

void initCel(int width, int height, int bpp, int type, CCB *cel);
void setupCelData(uint16 *pal, void *bitmap, CCB *cel);

int getCelWidth(CCB *cel);
int getCelHeight(CCB *cel);
int getCelBpp(CCB *cel);
int getCelType(CCB *cel);
uint16* getCelPalette(CCB *cel);
void* getCelBitmap(CCB *cel);

void setCelWidth(int width, CCB *cel);
void setCelHeight(int height, CCB *cel);
void setCelBpp(int bpp, CCB *cel);
void setCelType(int type, CCB *cel);
void setCelPalette(uint16 *pal, CCB *cel);
void setCelBitmap(void *bitmap, CCB *cel);

#endif
