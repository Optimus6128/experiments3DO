#include "core.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"

#include "mathutil.h"

#include "input.h"


#define FP_SHR 12
#define FP_MUL (1 << FP_SHR)

#define POW_TAB_RANGE 131072

static int powTab[POW_TAB_RANGE];
static int *powTabPtr = &powTab[POW_TAB_RANGE/2];

#define ESCAPE ((4 * FP_MUL) << FP_SHR)


#define FRAC_ITER_BLOCK0 \
						x1 = ((x0 * x0 - y0 * y0)>>FP_SHR) + xp0; \
						y1 = ((x0 * y0)>>(FP_SHR-1)) + yp0; \
						mj2 = x1 * x1 + y1 * y1; \
						x0 = x1; y0 = y1; c++;


#define FRAC_ITER_BLOCK \
						x1 = ((powTabPtr[x0] - powTabPtr[y0]) >> FP_SHR) + xp0; \
						y1 = ((x0 * y0)>>(FP_SHR-1)) + yp0; \
						mj2 = powTabPtr[x1] + powTabPtr[y1]; \
						x0 = x1; y0 = y1; c++;

#define FRAC_ITER_ESCAPE1 if (mj2 > ESCAPE) goto end1;
#define FRAC_ITER_ESCAPE2 if (mj2 > ESCAPE) goto end2;

#define FRAC_ITER_TIMES1_1 FRAC_ITER_BLOCK
#define FRAC_ITER_TIMES2_1 FRAC_ITER_BLOCK
#define FRAC_ITER_TIMES1_2b FRAC_ITER_BLOCK FRAC_ITER_ESCAPE1
#define FRAC_ITER_TIMES2_2b FRAC_ITER_BLOCK FRAC_ITER_ESCAPE2

#define FRAC_ITER_TIMES1_2 FRAC_ITER_BLOCK FRAC_ITER_ESCAPE1 FRAC_ITER_BLOCK FRAC_ITER_ESCAPE1
#define FRAC_ITER_TIMES1_4 FRAC_ITER_TIMES1_2 FRAC_ITER_TIMES1_2
#define FRAC_ITER_TIMES1_8 FRAC_ITER_TIMES1_4 FRAC_ITER_TIMES1_4
#define FRAC_ITER_TIMES1_16 FRAC_ITER_TIMES1_8 FRAC_ITER_TIMES1_8

#define FRAC_ITER_TIMES2_2 FRAC_ITER_BLOCK FRAC_ITER_ESCAPE2 FRAC_ITER_BLOCK FRAC_ITER_ESCAPE2
#define FRAC_ITER_TIMES2_4 FRAC_ITER_TIMES2_2 FRAC_ITER_TIMES2_2
#define FRAC_ITER_TIMES2_8 FRAC_ITER_TIMES2_4 FRAC_ITER_TIMES2_4
#define FRAC_ITER_TIMES2_16 FRAC_ITER_TIMES2_8 FRAC_ITER_TIMES2_8

#define JULIA_WIDTH 240
#define JULIA_HEIGHT 160

static Sprite *juliaSprUp;
static Sprite *juliaSprDown;
static uint16 juliaPal[16];
static unsigned char *juliaBmp;


static void juliaRender(int xp, int yp)
{
	int x,y,cv;

	const int di = 90;//(int)(0.022f * FP_MUL);

	int yk = -di * (JULIA_HEIGHT/2);
	int xl = di * -(JULIA_WIDTH/2) + (di / 2);
	
	const int xp0 = xp;
	const int yp0 = yp;

	unsigned char *dst = juliaBmp;
	for (y=0; y<JULIA_HEIGHT/2; ++y) {
		int xk = xl;
		for (x=0; x<JULIA_WIDTH/2; ++x) {
			int x0,y0;
			unsigned char c;
			int x1,y1,mj2;

			c = 255;
			x0 = xk; y0 = yk;
			
			FRAC_ITER_TIMES1_16
			end1:
			
			cv = (c << 4);
			xk+=di;

			c = 255;
			x0 = xk; y0 = yk;

			FRAC_ITER_TIMES2_16
			end2:

			cv |= c;
			xk+=di;

			*dst++ = cv;
		}
		yk+=di;
	}
}

static void initPowTab()
{
	int i;
	for (i=0; i<POW_TAB_RANGE; ++i) {
		powTab[i] = (i - POW_TAB_RANGE/2) * (i - POW_TAB_RANGE/2);
	}
}

void effectJuliaInit()
{
	juliaBmp = (unsigned char*)AllocMem((JULIA_WIDTH * JULIA_HEIGHT/2) / 2, MEMTYPE_ANY);
	juliaSprUp = newSprite(JULIA_WIDTH, JULIA_HEIGHT/2, 4, CEL_TYPE_CODED, juliaPal, juliaBmp);
	juliaSprDown = newSprite(JULIA_WIDTH, JULIA_HEIGHT/2, 4, CEL_TYPE_CODED, juliaPal, juliaBmp);

	setPalGradient(0,15, 0,0,1, 31,31,31, juliaPal);
	setSpritePosition(juliaSprUp, (SCREEN_WIDTH - JULIA_WIDTH) / 2, (SCREEN_HEIGHT - JULIA_HEIGHT) / 2);
	setSpritePosition(juliaSprDown, SCREEN_WIDTH - (SCREEN_WIDTH - JULIA_WIDTH) / 2, SCREEN_HEIGHT - (SCREEN_HEIGHT - JULIA_HEIGHT) / 2);
	flipSprite(juliaSprDown, true, true);

	initPowTab();
}

void effectJuliaRun()
{
	const int t = getTicks();
	const int xp = sin(t/800.0) * FP_MUL;
	const int yp = sin(t/1100.0) * FP_MUL;

	juliaRender(xp,yp);

	drawSprite(juliaSprUp);
	drawSprite(juliaSprDown);
}
