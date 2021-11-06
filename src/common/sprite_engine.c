#include "sprite_engine.h"
#include "cel_packer.h"
#include "system_graphics.h"
#include "tools.h"

static void rotatePoint(int *px, int *py, int angle)
{
	int tempX = *px;
	int tempY = *py;

	int isin = SinF16(angle);
	int icos = CosF16(angle);

	*px = ((tempX * icos + tempY * isin) >> 16);
	*py = ((tempX * isin - tempY * icos) >> 16);
}

Sprite *newSprite(int width, int height, int bpp, int type, uint16 *pal, ubyte *bmp)
{
	Sprite *spr = (Sprite*)AllocMem(sizeof(Sprite), MEMTYPE_ANY);

	spr->width = width;
	spr->height = height;

	spr->cel = CreateCel(width, height, bpp, type, bmp);
	spr->cel->ccb_SourcePtr = (void*)bmp;	// CreateCel is supposed to do that for you but fails for some pointer addresses, either a bug or something I miss

	spr->cel->ccb_Flags |= (CCB_ACSC | CCB_ALSC);
	if (type == CREATECEL_CODED) spr->cel->ccb_PLUTPtr = (PLUTChunk*)pal;

	spr->posX = spr->posY = 0;
	spr->angle = 0;
	spr->zoom = 256;

	return spr;
}

Sprite *newFeedbackSprite(int posX, int posY, int width, int height, int bufferIndex)
{
	Sprite *spr = newSprite(width, height, 16, CREATECEL_UNCODED, NULL, (ubyte*)getBackBuffer());

	setupWindowFeedbackCel(posX, posY, width, height, bufferIndex, spr->cel);

	return spr;
}

Sprite *newPackedSprite(int width, int height, int bpp, int type, uint16 *pal, ubyte *unpackedBmp, ubyte *packedData, int transparentColor)
{
	Sprite *spr;
	ubyte *providedPackedData = packedData;

	if (type == CREATECEL_CODED && !pal) return NULL;   // need palette to know when creating the packed sprite, which indexed color is transparent

	if (!providedPackedData) providedPackedData = createPackedDataFromUnpackedBmp(width, height, bpp, type, pal, unpackedBmp, transparentColor);

	spr = newSprite(width, height, bpp, type, pal, providedPackedData);

	spr->cel->ccb_Flags |= CCB_PACKED;

	return spr;
}

void setPalette(Sprite *spr, uint16* pal)
{
	spr->cel->ccb_PLUTPtr = (PLUTChunk*)pal;
}

void setSpriteAlpha(Sprite *spr, bool enable)
{
	if (enable)
		spr->cel->ccb_PIXC = 0x1F801F80;
	else
		spr->cel->ccb_PIXC = SOLID_CEL;
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

/*static void mapZoomSpriteCorner(Sprite *spr)   // could add enums in the future
{
	spr->cel->ccb_XPos = spr->posX << 16;
	spr->cel->ccb_YPos = spr->posY << 16;

	spr->cel->ccb_HDX = spr->zoom<<12;
	spr->cel->ccb_VDY = spr->zoom<<8;
}

static void mapZoomSpriteOffset(Sprite *spr, int px, int py)
{
	const int centerPixel = (spr->zoom >> 9) << 16;

	spr->cel->ccb_XPos = ((spr->posX - ((px * spr->zoom) >> 8)) << 16) - centerPixel;
	spr->cel->ccb_YPos = ((spr->posY - ((py * spr->zoom) >> 8)) << 16) - centerPixel;

	spr->cel->ccb_HDX = spr->zoom<<12;
	spr->cel->ccb_VDY = spr->zoom<<8;
}*/

static void mapZoomRotateSprite(Sprite *spr)
{
	int hdx = spr->zoom;
	int hdy = 0;
	int vdx = 0;
	int vdy = spr->zoom;

	rotatePoint(&hdx, &hdy, spr->angle);
	rotatePoint(&vdx, &vdy, spr->angle);

	spr->cel->ccb_HDX = hdx << 12;
	spr->cel->ccb_HDY = hdy << 12;
	spr->cel->ccb_VDX = vdx << 8;
	spr->cel->ccb_VDY = vdy << 8;

	hdx *= (spr->width >> 1);
	hdy *= (spr->width >> 1);
	vdx *= (spr->height >> 1);
	vdy *= (spr->height >> 1);

	spr->cel->ccb_XPos = (spr->posX << 16) + ((- hdx - vdx) << 8);
	spr->cel->ccb_YPos = (spr->posY << 16) + ((- hdy - vdy) << 8);
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

void *getSpriteBitmapData(Sprite *spr)
{
	return spr->cel->ccb_SourcePtr;
}

void drawSprite(Sprite *spr)
{
	drawCels(spr->cel);
}
