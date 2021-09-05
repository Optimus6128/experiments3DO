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

void setPalette(Sprite *spr, uint16* pal);
void setSpriteAlpha(Sprite *spr, bool enabled);

void mapSprite(Sprite *spr);
void mapZoomSprite(Sprite *spr);
void mapZoomSpriteCorner(Sprite *spr);
void mapZoomSpriteOffset(Sprite *spr, int px, int py);
void mapZoomRotateSprite(Sprite *spr);

void mapStretchSpriteX(Sprite *spr);
void mapStretchSpriteY(Sprite *spr);

void drawSprite(Sprite *spr);

#endif
