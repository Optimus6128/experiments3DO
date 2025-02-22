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
	// Zoom and Angle are 20:8
	// zoom : 1<<8 means 1x, 2<<8 means 2x, 128 means x0.5
	// angle: 1<<8 means 1 degree, 90<<8 is right angle

	uint16 *pal;
	void *data;
	CCB *cel;
	Point quad[4];
}Sprite;


Sprite *newSprite(int width, int height, int bpp, int type, uint16 *pal, unsigned char *bmp);
Sprite *newPackedSprite(int width, int height, int bpp, int type, uint16 *pal, unsigned char *unpackedBmp, unsigned char *packedData, int transparentColor);
Sprite *newFeedbackSprite(int posX, int posY, int width, int height, int bufferIndex);
Sprite *loadSpriteCel(char *path);

void setSpritePalette(Sprite *spr, uint16* pal);
void setSpriteData(Sprite *spr, void *data);
void setSpriteAlpha(Sprite *spr, bool enable, bool average);
void setSpriteDottedDisplay(Sprite *spr, bool enable);

void setSpritePosition(Sprite *spr, int px, int py);
void setSpritePositionZoom(Sprite *spr, int px, int py, int zoom);
void setSpritePositionZoomRotate(Sprite *spr, int px, int py, int zoom, int angle);
void flipSprite(Sprite *spr, bool horizontal, bool vertical);

void mapZoomSpriteToQuad(Sprite *spr, int ulX, int ulY, int lrX, int lrY);
void mapFeedbackSpriteToNewFramebufferArea(int ulX, int ulY, int lrX, int lrY, int bufferIndex, Sprite *spr);

void *getSpriteBitmapData(Sprite *spr);

void drawSprite(Sprite *spr);

#endif
