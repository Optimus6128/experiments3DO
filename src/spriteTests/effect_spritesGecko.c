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

static void animateGeckoCels(Point2D *center, Point2D *zoom, int angle)	// 0-255 for 360 angle, zoom X1 = 256
{
	int x,y;
	int i = 0;

	const int dx = SPR_W * CosF16(angle<<16);
	const int dy = SPR_H * -SinF16(angle<<16);

	const int vvx = (dx * zoom->x) >> 8;
	const int vvy = (dy * zoom->x) >> 8;
	const int vx = (dx * zoom->y) >> 8;
	const int vy = (dy * zoom->y) >> 8;
	
	const int chx = -mg_width / 2;
	const int chy = -mg_height / 2;

	int px = (center->x << 16) + chx * vvx - chy * vy;
	int py = (center->y << 16) + chx * vvy + chy * vx;
	
	const int countX = mg_width;
	const int countY = mg_height;

	for (y=0; y<countY; ++y) {
		int ppx = px;
		int ppy = py;

		for (x=0; x<countX; ++x) {
			microGex[i]->ccb_XPos = ppx;
			microGex[i]->ccb_YPos = ppy;

			ppx += vvx;
			ppy += vvy;

			++i;
		}
		px -= vy;
		py += vx;
	}
}

static void scriptGeckoCels()
{
	const int t = getTicks();

	Point2D center;
	Point2D zoom;

	const int angle = SinF16(t<<10) >> 8;

	center.x = SCREEN_WIDTH / 2 - SPR_W / 2;
	center.y = SCREEN_HEIGHT / 2 - SPR_H / 2;
	zoom.x = 256 + (SinF16(t<<11) >> 9);
	zoom.y = 384 + (SinF16(t<<12) >> 8);

	animateGeckoCels(&center, &zoom, angle);
}

void effectSpritesGeckoInit()
{
	loadAndInitGeckoCels();
	prepareGeckoCels();
}

void effectSpritesGeckoRun()
{
	scriptGeckoCels();

	drawCels(microGex[0]);
}
