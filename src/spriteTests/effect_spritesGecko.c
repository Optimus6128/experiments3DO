#include "core.h"

#include "system_graphics.h"
#include "tools.h"

#include "mathutil.h"
#include "celutils.h"

#define SPR_W 4
#define SPR_H 4

//#define SINGLE_DRAW_CEL_CALLS

uint16 *geckoCelStorage;

CCB *geckoCel;
CCB *microGexCels;
int mg_width, mg_height;

static void loadAndInitGeckoCels()
{
	geckoCel = LoadCel("data/gecko2.cel", MEMTYPE_CEL);	// original CEL image is 192x160, split this in 48x40 regions of size 4x4 (SPR_W * SPR_H)

	mg_width = geckoCel->ccb_Width / SPR_W;
	mg_height = geckoCel->ccb_Height / SPR_H;

	microGexCels = createCels(SPR_W, SPR_H, 16, CEL_TYPE_UNCODED, mg_width * mg_height);
}

static void copyGeckoCels()
{
	int x,y;
	int i,j;

	uint16 *dst = geckoCelStorage = (uint16*)AllocMem(mg_width * mg_height * SPR_W * SPR_H * 2, MEMTYPE_ANY);

	for (y=0; y<geckoCel->ccb_Height; y+=SPR_H) {
		for (x=0; x<geckoCel->ccb_Width; x+=SPR_W) {
			uint16 *src = (uint16*)geckoCel->ccb_SourcePtr + y * geckoCel->ccb_Width + x;
			for (j=0; j<SPR_H; ++j) {
				for (i=0; i<SPR_W; ++i) {
					*dst++ = *(src + j * geckoCel->ccb_Width + i);
				}
			}
		}
	}
}

static void prepareGeckoCels()
{
	int x,y;
	int i=0;

	for (y=0; y<geckoCel->ccb_Height; y+=SPR_H) {
		for (x=0; x<geckoCel->ccb_Width; x+=SPR_W) {
			uint16 *dstPtr = geckoCelStorage + i * SPR_W * SPR_H;
			setupCelData(NULL, dstPtr, &microGexCels[i]);

			microGexCels[i].ccb_XPos = x << 16;
			microGexCels[i].ccb_YPos = y << 16;
			microGexCels[i].ccb_Flags |= CCB_BGND;
			if (i>0) {
				#ifndef SINGLE_DRAW_CEL_CALLS
					linkCel(&microGexCels[i-1], &microGexCels[i]);
				#endif
				microGexCels[i].ccb_Flags &= ~(CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP);
				memcpy(&microGexCels[i].ccb_HDX, &microGexCels[i].ccb_PRE0, 8);
			}
			++i;
		}
	}
}

static void animateGeckoCels(Point2D *center, Point2D *zoom, int angle)	// 0-255 for 360 angle, zoom X1 = 256
{
	//int x;
	int y;

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
	
	//const int countX = mg_width;
	const int countY = mg_height;

	CCB *mgex = microGexCels;
	for (y=0; y<countY; ++y) {
		int ppx = px;
		int ppy = py;

		//for (x=0; x<countX; ++x) {
		// UNROLL 48 times

			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;

			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;

			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			ppx += vvx; ppy += vvy; mgex++;
			mgex->ccb_XPos = ppx; mgex->ccb_YPos = ppy;
			/*ppx += vvx; ppy += vvy;*/ mgex++;
		//}
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
	copyGeckoCels();
	prepareGeckoCels();
}

#ifdef SINGLE_DRAW_CEL_CALLS
static void drawCelsManyCalls()
{
	int i;
	const int count = mg_width * mg_height;

	for (i=0; i<count; ++i) {
		drawCels(&microGexCels[i]);
	}
}
#endif

void effectSpritesGeckoRun()
{
	scriptGeckoCels();

	#ifdef SINGLE_DRAW_CEL_CALLS
		drawCelsManyCalls();
	#else
		drawCels(microGexCels);
	#endif
}
