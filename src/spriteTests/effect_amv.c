#include <math.h>

#include "core.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "tools.h"

#include "mathutil.h"

#include "input.h"


#define SPRITE_WIDTH 64
#define SPRITE_HEIGHT 64
#define CIRCLE_RADIUS (SPRITE_WIDTH/2-1)

static Sprite *circleSpr;
static uint16 circlePal[32];
static unsigned char *circleBmp;

//0x1F001F00
//((SRC1(Texture) * 8) / 8 + 0) / 1
//((SRC1(Texture) * 4) / 4 + 0) / 1
//((SRC1(Texture) * 2) / 2 + 0) / 1
//((SRC1(Texture) * 1) / 1 + 0) / 1

//MMM DD
//00011111	// 1F
//00001110	// 0E
//00000101	// 05
//00000000	// 00

//0x1F001F00
//0x0E000E00
//0x05000500
//0x00000000

//PIXC = 0

//0x1F801F80
//((SRC1(Texture) * 8) / 8 + SRC2(FrameBuffer)) / 1

//0x1F811F81
//((SRC1(Texture) * 8) / 8 + SRC2(FrameBuffer)) / 2


//( (SRC1(Texture) * AMV(1-8)) / PDV(8) + SRC2(ZERO) ) / SDV(1)

//0010001100000000

//( (SRC1(Texture) * AMV(1-8)) / PDV(8) + SRC2(FRAMEBUFFER) ) / SDV(2)

//0010001110000001

//(1/8th of Texture + Background) / 2

//0x23002300
//0x23812381

//#define CEL_BLEND_OPAQUE 0x1F001F00
//#define CEL_BLEND_ADDITIVE 0x1F801F80
//#define CEL_BLEND_AVERAGE 0x1F811F81

static void initCircle()
{
	int x,y;

	unsigned char *dst = circleBmp;

	for (y=-SPRITE_HEIGHT/2; y<SPRITE_HEIGHT/2; ++y) {
		for (x=-SPRITE_WIDTH/2; x<SPRITE_WIDTH/2; ++x) {
			const float r = (float)sqrt((float)(x*x + y*y));
			if (r < CIRCLE_RADIUS) {
				int c = ((x+SPRITE_WIDTH/2)^(y+SPRITE_HEIGHT/2)) % 31;
				CLAMP(c, 1,30);
				*dst++ = c;
			} else if (r < CIRCLE_RADIUS + 1) {
				const float d = CIRCLE_RADIUS + 1 - r;
				// WILL use AMV bits to shade antialiased pixels
				int c = (int)(8 * d);
				CLAMP(c, 0,7)
				*dst++ = 31 | (c << 5);

				//*dst++ = (unsigned char)(31 * d);
			} else {
				*dst++ = 0;
			}
		}
	}
}

void effectAmvInit()
{
	int i;
	const int size = SPRITE_WIDTH * SPRITE_HEIGHT;

	circleBmp = (unsigned char*)AllocMem(size, MEMTYPE_ANY);
	memset(circleBmp, 0, size);
	circleSpr = newSprite(SPRITE_WIDTH, SPRITE_HEIGHT, 8, CEL_TYPE_CODED, circlePal, circleBmp);

	setPalGradient(0,31, 0,0,0, 15,23,31, circlePal);

	for (i=0; i<31; ++i) {
		circlePal[i] |= (1 << 15);
	}

	initCircle();

	//circleSpr->cel->ccb_PIXC = 0x23002300;
	//circleSpr->cel->ccb_PIXC = 0x1F002380;
	circleSpr->cel->ccb_PIXC = 0x1F002381;
	//circleSpr->cel->ccb_PIXC = 0x23802381;

	loadAndSetBackgroundImage("data/background.img", getBackBuffer());
}

void effectAmvRun()
{
	const int t = getTicks() / 2;
	const int xp = SCREEN_WIDTH / 2 + (int)(sin(t/1132.0) * 64) - SPRITE_WIDTH/2;
	const int yp = SCREEN_HEIGHT / 2 + (int)(sin(t/948.0) * 48) - SPRITE_HEIGHT/2;

	setSpritePositionZoom(circleSpr, xp, yp, 1024);
	drawSprite(circleSpr);
}
