#ifndef SPRITE_ENGINE_H
#define SPRITE_ENGINE_H

#include "types.h"
#include "core.h"

typedef struct Sprite
{
	int width;
	int height;

	int posX, posY;
	int angle, zoom;

	CCB *cel;
	Point quad[4];
}Sprite;


Sprite *newSprite(int width, int height, int bpp, int type, uint16 *pal, ubyte *bmp);
Sprite *newPackedSprite(int width, int height, int bpp, int type, uint16 *pal, ubyte *unpackedBmp, ubyte *packedData, int transparentColor);
Sprite *newFeedbackSprite(int posX, int posY, int width, int height, int bufferIndex);

void setPalette(Sprite *spr, uint16* pal);
void setSpriteAlpha(Sprite *spr, bool enable);
void setSpriteDottedDisplay(Sprite *spr, bool enable);

void setSpritePosition(Sprite *spr, int px, int py);
void setSpritePositionZoom(Sprite *spr, int px, int py, int zoom);
void setSpritePositionZoomRotate(Sprite *spr, int px, int py, int zoom, int angle);

void *getSpriteBitmapData(Sprite *spr);

void drawSprite(Sprite *spr);

#endif
