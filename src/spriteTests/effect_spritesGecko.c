#include "core.h"

#include "effect_spritesGecko.h"

#include "system_graphics.h"
#include "tools.h"

#include "mathutil.h"
#include "celutils.h"

#define SPR_W 4
#define SPR_H 4

CCB *geckoCel;
CCB **microGex;
int mg_width, mg_height;

void effectSpritesGeckoInit()
{
	int x,y;
	int i=0;

int fuck;

	geckoCel = LoadCel("data/gecko2.cel", MEMTYPE_CEL);	// original CEL image is 192x160, split this in 48x40 regions of size 4x4 (SPR_W * SPR_H)

	mg_width = geckoCel->ccb_Width / SPR_W;
	mg_height = geckoCel->ccb_Height / SPR_H;

	microGex = (CCB**)AllocMem(sizeof(CCB*) * (mg_width * mg_height), MEMTYPE_ANY);

	for (y=0; y<geckoCel->ccb_Height; y+=SPR_H) {
		for (x=0; x<geckoCel->ccb_Width; x+=SPR_W) {
			int *dstPtr = (int*)geckoCel->ccb_SourcePtr;
			dstPtr += ((y * geckoCel->ccb_Width + x)>>1);

			microGex[i] = CreateCel(SPR_W, SPR_H, 16, CREATECEL_UNCODED, dstPtr);
			microGex[i]->ccb_SourcePtr = (CelData*)dstPtr;	// fuck libs, let's reinvent the wheel ALWAYS
			microGex[i]->ccb_PRE1 = (microGex[i]->ccb_PRE1 & ~PRE1_WOFFSET10_MASK) | (((geckoCel->ccb_Width >> 1) - 2) << 16);
			microGex[i]->ccb_XPos = x << 16;
			microGex[i]->ccb_YPos = y << 16;
			if (i>0) LinkCel(microGex[i-1], microGex[i]);
			++i;
		}
	}
}

void effectSpritesGeckoRun()
{
	drawCels(microGex[0]);
}
