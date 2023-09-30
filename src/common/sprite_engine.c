#include "sprite_engine.h"
#include "cel_packer.h"
#include "system_graphics.h"
#include "cel_helpers.h"


Sprite *newSprite(int width, int height, int bpp, int type, uint16 *pal, unsigned char *bmp)
{
	Sprite *spr = (Sprite*)AllocMem(sizeof(Sprite), MEMTYPE_ANY);

	spr->width = width;
	spr->height = height;

	spr->cel = createCel(width, height, bpp, type);
	setupCelData(pal, bmp, spr->cel);

	spr->posX = spr->posY = 0;
	spr->angle = 0;
	spr->zoom = 256;

	spr->data = bmp;
	spr->pal = pal;

	return spr;
}

Sprite *loadSpriteCel(char *path)
{
	Sprite *spr = (Sprite*)AllocMem(sizeof(Sprite), MEMTYPE_ANY);
	CCB *sprCel = LoadCel(path, MEMTYPE_ANY);

	spr->cel = sprCel;
	spr->width = sprCel->ccb_Width;
	spr->height = sprCel->ccb_Height;

	spr->posX = spr->posY = 0;
	spr->angle = 0;
	spr->zoom = 256;

	spr->data = getCelBitmap(sprCel);
	spr->pal = getCelPalette(sprCel);

	return spr;
}

Sprite *newFeedbackSprite(int posX, int posY, int width, int height, int bufferIndex)
{
	Sprite *spr = newSprite(width, height, 16, CEL_TYPE_UNCODED, NULL, (unsigned char*)getBackBuffer());

	setupWindowFeedbackCel(posX, posY, width, height, bufferIndex, spr->cel);

	return spr;
}

Sprite *newPackedSprite(int width, int height, int bpp, int type, uint16 *pal, unsigned char *unpackedBmp, unsigned char *packedData, int transparentColor)
{
	Sprite *spr;
	unsigned char *providedPackedData = packedData;

	if (type == CEL_TYPE_CODED && !pal) return NULL;   // need palette to know when creating the packed sprite, which indexed color is transparent

	if (!providedPackedData) providedPackedData = createPackedDataFromUnpackedBmp(width, height, bpp, type, pal, unpackedBmp, transparentColor);

	spr = newSprite(width, height, bpp, type, pal, providedPackedData);

	spr->cel->ccb_Flags |= CCB_PACKED;

	return spr;
}

void setPalette(Sprite *spr, uint16* pal)
{
	spr->cel->ccb_PLUTPtr = pal;
}

void setSpriteAlpha(Sprite *spr, bool enable, bool average)
{
	if (enable) {
		if (average) {
			spr->cel->ccb_PIXC = CEL_BLEND_AVERAGE;
		} else {
			spr->cel->ccb_PIXC = CEL_BLEND_ADDITIVE;
		}
	} else {
		spr->cel->ccb_PIXC = CEL_BLEND_OPAQUE;
	}
}

void setSpriteDottedDisplay(Sprite *spr, bool enable)
{
	if (enable)
		spr->cel->ccb_Flags |= CCB_MARIA;
	else
		spr->cel->ccb_Flags &= ~CCB_MARIA;
}


static void mapSprite(Sprite *spr)
{
	spr->cel->ccb_XPos = spr->posX << 16;
	spr->cel->ccb_YPos = spr->posY << 16;
}

static void mapZoomSprite(Sprite *spr)
{
	spr->cel->ccb_XPos = (spr->posX - (((spr->width >> 1) * spr->zoom) >> 8)) << 16;
	spr->cel->ccb_YPos = (spr->posY - (((spr->height >> 1) * spr->zoom) >> 8)) << 16;

	spr->cel->ccb_HDX = spr->zoom<<12;
	spr->cel->ccb_VDY = spr->zoom<<8;
}

static void mapZoomRotateSprite(Sprite *spr)
{
	int hdx = (spr->zoom * CosF16(spr->angle<<8)) >> 16;
	int hdy = (spr->zoom * -SinF16(spr->angle<<8)) >> 16;
	int vdx = -hdy;
	int vdy = hdx;

	spr->cel->ccb_HDX = hdx << 12;
	spr->cel->ccb_HDY = hdy << 12;
	spr->cel->ccb_VDX = vdx << 8;
	spr->cel->ccb_VDY = vdy << 8;

	// sub to move to center of the sprite
	hdx *= (spr->width >> 1);
	hdy *= (spr->width >> 1);
	vdx *= (spr->height >> 1);
	vdy *= (spr->height >> 1);
	spr->cel->ccb_XPos = (spr->posX << 16) + ((- hdx - vdx) << 8);
	spr->cel->ccb_YPos = (spr->posY << 16) + ((- hdy - vdy) << 8);
}

void mapZoomSpriteToQuad(Sprite *spr, int ulX, int ulY, int lrX, int lrY)
{
	spr->cel->ccb_XPos = ulX << 16;
	spr->cel->ccb_YPos = ulY << 16;

	// Could be faster if they were power of two, but now it's a quick hack
	spr->cel->ccb_HDX = ((lrX-ulX+1) << 20) / spr->width;
	spr->cel->ccb_HDY = 0;
	spr->cel->ccb_VDX = 0;
	spr->cel->ccb_VDY = ((lrY-ulY+1) << 16) / spr->height;
}

void mapFeedbackSpriteToNewFramebufferArea(int ulX, int ulY, int lrX, int lrY, int bufferIndex, Sprite *spr)
{
	const int newWidth = (lrX-ulX+1) & ~1;
	const int newHeight = (lrY-ulY+1) & ~1;

	spr->width = newWidth;
	spr->height = newHeight;

	setupWindowFeedbackCel(ulX, ulY, newWidth, newHeight, bufferIndex, spr->cel);
}

void setSpritePosition(Sprite *spr, int px, int py)
{
	spr->posX = px;
	spr->posY = py;

	mapSprite(spr);
}

void setSpritePositionZoom(Sprite *spr, int px, int py, int zoom)
{
	spr->posX = px;
	spr->posY = py;
	spr->zoom = zoom;

	mapZoomSprite(spr);
}

void setSpritePositionZoomRotate(Sprite *spr, int px, int py, int zoom, int angle)
{
	spr->posX = px;
	spr->posY = py;
	spr->zoom = zoom;
	spr->angle = angle;

	mapZoomRotateSprite(spr);
}

void flipSprite(Sprite *spr, bool horizontal, bool vertical)
{
	flipCelOrientation(horizontal, vertical, spr->cel);
}

void *getSpriteBitmapData(Sprite *spr)
{
	return spr->data;
}

void drawSprite(Sprite *spr)
{
	drawCels(spr->cel);
}
