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

static void loadAndInitGeckoCels()
{
	int i;
	int celsNum;

	geckoCel = LoadCel("data/gecko2.cel", MEMTYPE_CEL);	// original CEL image is 192x160, split this in 48x40 regions of size 4x4 (SPR_W * SPR_H)

	mg_width = geckoCel->ccb_Width / SPR_W;
	mg_height = geckoCel->ccb_Height / SPR_H;

	celsNum = mg_width * mg_height;
	microGex = (CCB**)AllocMem(sizeof(CCB*) * celsNum, MEMTYPE_ANY);

	for (i=0; i< celsNum; ++i) {
		microGex[i] = createCel(SPR_W, SPR_H, 16, CEL_TYPE_UNCODED);
	}
}

static void prepareGeckoCels()
{
	int x,y;
	int i=0;

	for (y=0; y<geckoCel->ccb_Height; y+=SPR_H) {
		for (x=0; x<geckoCel->ccb_Width; x+=SPR_W) {
			uint16 *dstPtr = (uint16*)geckoCel->ccb_SourcePtr + y * geckoCel->ccb_Width + x;
			setupCelData(NULL, dstPtr, microGex[i]);

			microGex[i]->ccb_PRE1 = (microGex[i]->ccb_PRE1 & ~PRE1_WOFFSET10_MASK) | (((geckoCel->ccb_Width >> 1) - 2) << 16);
			microGex[i]->ccb_XPos = x << 16;
			microGex[i]->ccb_YPos = y << 16;
			if (i>0) linkCel(microGex[i-1], microGex[i]);
			++i;
		}
	}
}

void effectSpritesGeckoInit()
{
	loadAndInitGeckoCels();
	prepareGeckoCels();
}

void effectSpritesGeckoRun()
{
	drawCels(microGex[0]);
}
