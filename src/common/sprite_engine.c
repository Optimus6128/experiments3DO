#include "sprite_engine.h"
#include "cel_packer.h"
#include "system_graphics.h"

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
	Sprite *spr = (Sprite*)malloc(sizeof(Sprite));

	spr->width = width;
	spr->height = height;

	spr->cel = CreateCel(width, height, bpp, type, bmp);

	spr->cel->ccb_Flags |= (CCB_ACSC | CCB_ALSC);
	if (type == CREATECEL_CODED) spr->cel->ccb_PLUTPtr = (PLUTChunk*)pal;

	spr->posX = spr->posY = 0;
	spr->angle = 0;
	spr->zoom = 256;

	return spr;
}

Sprite *newPackedSprite(int width, int height, int bpp, int type, uint16 *pal, ubyte *unpackedBmp, ubyte *packedData)
{
	Sprite *spr;

	if (type == CREATECEL_CODED && !pal) return NULL;   // need palette to know when creating the packed sprite, which indexed color is transparent

	if (!packedData) packedData = createPackedDataFromUnpackedBmp(width, height, bpp, type, pal, unpackedBmp);

	spr = newSprite(width, height, bpp, type, pal, packedData);

	spr->cel->ccb_Flags |= CCB_PACKED;

	return spr;
}

void setPalette(Sprite *spr, uint16* pal)
{
	spr->cel->ccb_PLUTPtr = (PLUTChunk*)pal;
}

void setSpriteAlpha(Sprite *spr, bool enabled)
{
	if (enabled)
		spr->cel->ccb_PIXC = 0x1F801F80;
	else
		spr->cel->ccb_PIXC = SOLID_CEL;
}

void mapSprite(Sprite *spr)
{
	spr->cel->ccb_XPos = spr->posX << 16;
	spr->cel->ccb_YPos = spr->posY << 16;
}

void mapStretchSpriteX(Sprite *spr)
{
	spr->cel->ccb_XPos = (spr->posX - (((spr->width >> 1) * spr->zoom) >> 8)) << 16;
	spr->cel->ccb_YPos = (spr->posY - (spr->height >> 1)) << 16;

	spr->cel->ccb_HDX = spr->zoom<<12;
	spr->cel->ccb_VDY = 1<<16;
}

void mapStretchSpriteY(Sprite *spr)
{
	spr->cel->ccb_XPos = (spr->posX - (spr->width >> 1)) << 16;
	spr->cel->ccb_YPos = (spr->posY - (((spr->height >> 1) * spr->zoom) >> 8)) << 16;

	spr->cel->ccb_HDX = 1<<20;
	spr->cel->ccb_VDY = spr->zoom<<8;
}

void mapZoomSprite(Sprite *spr)
{
	spr->cel->ccb_XPos = (spr->posX - (((spr->width >> 1) * spr->zoom) >> 8)) << 16;
	spr->cel->ccb_YPos = (spr->posY - (((spr->height >> 1) * spr->zoom) >> 8)) << 16;

	spr->cel->ccb_HDX = spr->zoom<<12;
	spr->cel->ccb_VDY = spr->zoom<<8;
}

void mapZoomSpriteCorner(Sprite *spr)   // could add enums in the future
{
	spr->cel->ccb_XPos = spr->posX << 16;
	spr->cel->ccb_YPos = spr->posY << 16;

	spr->cel->ccb_HDX = spr->zoom<<12;
	spr->cel->ccb_VDY = spr->zoom<<8;
}

void mapZoomSpriteOffset(Sprite *spr, int px, int py)
{
	const int centerPixel = (spr->zoom >> 9) << 16;

	spr->cel->ccb_XPos = ((spr->posX - ((px * spr->zoom) >> 8)) << 16) - centerPixel;
	spr->cel->ccb_YPos = ((spr->posY - ((py * spr->zoom) >> 8)) << 16) - centerPixel;

	spr->cel->ccb_HDX = spr->zoom<<12;
	spr->cel->ccb_VDY = spr->zoom<<8;
}

void mapZoomRotateSprite(Sprite *spr)
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

void drawSprite(Sprite *spr)
{
	drawCels(spr->cel);
}
