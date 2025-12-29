#ifndef CEL_HELPERS_H
#define CEL_HELPERS_H

#include "types.h"
#include "core.h"

enum {
	CEL_TYPE_CODED = 0,
	CEL_TYPE_UNCODED = 1,
	CEL_TYPE_PACKED = 2,
	CEL_TYPE_ALLOC_PAL = 4,
	CEL_TYPE_ALLOC_BMP = 8
};

#define CEL_BLEND_OPAQUE 0x1F001F00
#define CEL_BLEND_ADDITIVE 0x1F801F80
#define CEL_BLEND_AVERAGE 0x1F811F81

void initCel(int width, int height, int bpp, int type, CCB *cel);
CCB *createCel(int width, int height, int bpp, int type);
CCB *createCels(int width, int height, int bpp, int type, int num);
void setupCelData(uint16 *pal, void *bitmap, CCB *cel);

int getCelWidth(CCB *cel);
int getCelHeight(CCB *cel);
int getCelBpp(CCB *cel);
int getCelType(CCB *cel);
uint16* getCelPalette(CCB *cel);
void* getCelBitmap(CCB *cel);

void setCelBpp(int bpp, CCB *cel);
void setCelFlags(CCB *cel, uint32 flags, bool enable);
void setCelType(int type, CCB *cel);
void setCelPalette(uint16 *pal, CCB *cel);
void setCelBitmap(void *bitmap, CCB *cel);
void setCelPosition(int x, int y, CCB *cel);

void flipCelOrientation(bool horizontal, bool vertical, CCB *cel);
void rotateCelOrientation(CCB *cel);

void setCelWidth(int width, CCB *cel);
void setCelHeight(int height, CCB *cel);
void setCelStride(int stride, CCB *cel);

void linkCel(CCB *ccb, CCB *nextCCB);

int getCelDataSizeInBytes(CCB *cel);
int getCelPaletteColorsRealBpp(int bpp);
void updateWindowCel(int posX, int posY, int width, int height, int *bitmap, CCB *cel);
void setupWindowFeedbackCel(int posX, int posY, int width, int height, int bufferIndex, CCB *cel);

#endif
