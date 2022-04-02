#include "core.h"

#include "tools.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"

#include "sprite_engine.h"

#include "system_graphics.h"

#include "cel_helpers.h"
#include "mathutil.h"
#include "tools.h"


#define SOFT_BUFF_WIDTH SCREEN_WIDTH
#define SOFT_BUFF_HEIGHT SCREEN_HEIGHT

#define DIV_TAB_SIZE 4096
#define DIV_TAB_SHIFT 16

// Semisoft gouraud method
#define MAX_SCANLINES 4096
static CCB *scanlineCel8[MAX_SCANLINES];
static CCB **currentScanlineCel8 = scanlineCel8;

#define GRADIENT_SHADES 32
#define GRADIENT_LENGTH GRADIENT_SHADES
#define GRADIENT_GROUP_SIZE (GRADIENT_SHADES * GRADIENT_LENGTH)
static uint8 gourGrads[GRADIENT_SHADES * GRADIENT_GROUP_SIZE];


typedef struct Edge
{
	int x;
	int c;
	int u,v;
}Edge;

static int renderSoftMethod = RENDER_SOFT_METHOD_GOURAUD;

static Sprite *sprSoftBuffer = NULL;
static Sprite *sprSoftBuffer8 = NULL;
static Sprite *sprSoftBuffer16 = NULL;

static uint8 softBuffer8[SOFT_BUFF_WIDTH * SOFT_BUFF_HEIGHT];
static uint16 softBuffer16[SOFT_BUFF_WIDTH * SOFT_BUFF_HEIGHT];

static Edge leftEdge[SOFT_BUFF_HEIGHT];
static Edge rightEdge[SOFT_BUFF_HEIGHT];

static int32 divTab[DIV_TAB_SIZE];

static uint16 *lineColorShades[4] = { NULL, NULL, NULL, NULL };
static uint16 *gouraudColorShades;

static uint16 *activeGradient = NULL;
static Texture *activeTexture = NULL;

#define LN_BASE 8
#define LN_AND ((1 << LN_BASE) - 1)


static void(*fillEdges)(int yMin, int yMax);
static void(*prepareEdgeList) (ScreenElement *e0, ScreenElement *e1);


static void bindGradient(uint16 *gradient)
{
	activeGradient = gradient;
}

static void bindTexture(Texture *texture)
{
	activeTexture = texture;
}

static uint16 *crateColorShades(int r, int g, int b, int numShades, bool absoluteZero) {
	uint16 *colorShades = (uint16*)AllocMem(sizeof(uint16) * numShades, MEMTYPE_ANY);
	int lb = 0;
	if (!absoluteZero) lb = 1;

	setPalGradient(0, numShades-1, 0,0,lb, r,g,b, colorShades);

	return colorShades;
}

static void initDivs()
{
    int i, ii;
    for (i=0; i<DIV_TAB_SIZE; ++i) {
        ii = i - DIV_TAB_SIZE / 2;
        if (ii==0) ++ii;

        divTab[i] = (1 << DIV_TAB_SHIFT) / ii;
    }
}

static void initSemiSoftGouraud()
{
	int i;
	uint8 *dst = gourGrads;
	int c0,c1,x;
	for (i=0; i<MAX_SCANLINES; ++i) {
		scanlineCel8[i] = createCel(GRADIENT_LENGTH, 1, 8, CEL_TYPE_CODED);
		if (i>0) {
			linkCel(scanlineCel8[i-1], scanlineCel8[i]);
			scanlineCel8[i]->ccb_Flags |= CCB_BGND;
			scanlineCel8[i]->ccb_Flags &= ~(CCB_LDPLUT | CCB_LDPRS | CCB_LDPPMP);
			memcpy(&scanlineCel8[i]->ccb_HDDX, &scanlineCel8[i]->ccb_PRE0, 8);
		}
	}

	for (c0=0; c0<GRADIENT_SHADES; ++c0) {
		for (c1=0; c1<GRADIENT_SHADES; ++c1) {
			const int repDiv = divTab[GRADIENT_LENGTH + DIV_TAB_SIZE / 2];
			const int dc = ((c1 - c0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
			int fc = INT_TO_FIXED(c0, FP_BASE);
			for (x=0; x<GRADIENT_LENGTH; ++x) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				CLAMP(c, 0, 31)
				*dst++ = c;
				fc += dc;
			}
		}
	}
}

static void drawAntialiasedLine(ScreenElement *e1, ScreenElement *e2)
{
	int x1 = e1->x;
	int y1 = e1->y;
	int x2 = e2->x;
	int y2 = e2->y;

	int dx, dy, l;
	int x00, y00;
	int vramofs;

	int x, y;
	int frac, shade;

	int temp;
    int chdx, chdy;

	uint16 *vram = (uint16*)softBuffer16;

    // ==== Clipping ====

    int outcode1 = 0, outcode2 = 0;

    if (y1 < 1) outcode1 |= 0x0001;
        else if (y1 > SOFT_BUFF_HEIGHT-2) outcode1 |= 0x0010;
    if (x1 < 1) outcode1 |= 0x0100;
        else if (x1 > SOFT_BUFF_WIDTH-2) outcode1 |= 0x1000;

    if (y2 < 1) outcode2 |= 0x0001;
        else if (y2 > SOFT_BUFF_HEIGHT-2) outcode2 |= 0x0010;
    if (x2 < 1) outcode2 |= 0x0100;
        else if (x2 > SOFT_BUFF_WIDTH-2) outcode2 |= 0x1000;

    if ((outcode1 & outcode2)!=0) return;

    //if ((outcode1 | outcode2)!=0) return; // normally, should check for possible clip
	//I will do lame method now

    // ==================

	dx = x2 - x1;
	dy = y2 - y1;

	if (dx==0 && dy==0) return;

    chdx = dx;
	chdy = dy;
    if (dx<0) chdx = -dx;
    if (dy<0) chdy = -dy;

	if (chdy < chdx) {
		if (x1 > x2) {
			temp = x1; x1 = x2; x2 = temp;
			y1 = y2;
		}

		if (dx==0) return;
        l = (dy << LN_BASE) / dx;
        y00 = y1 << LN_BASE;
		for (x=x1; x<x2; x++) {
			const int yp = y00 >> LN_BASE;

			if (x >= 0 && x < SOFT_BUFF_WIDTH && yp >=0 && yp < SOFT_BUFF_HEIGHT - 1) {
				vramofs = yp*SOFT_BUFF_WIDTH + x;
				frac = y00 & LN_AND;

				shade = (LN_AND - frac) >> 4;
				*(vram + vramofs) |= activeGradient[shade];

				shade = frac >> 4;
				*(vram + vramofs+SOFT_BUFF_WIDTH) |= activeGradient[shade];
			}
            y00+=l;
		}
	}
	else {
		if (y1 > y2) {
			temp = y1; y1 = y2; y2 = temp;
			x1 = x2;
		}

		if (dy==0) return;
        l = (dx << LN_BASE) / dy;
        x00 = x1 << LN_BASE;

		for (y=y1; y<y2; y++) {
			const int xp = x00 >> LN_BASE;

			if (y >= 0 && y < SOFT_BUFF_HEIGHT && xp >=0 && xp < SOFT_BUFF_WIDTH - 1) {
				vramofs = y*SOFT_BUFF_WIDTH + xp;
				frac = x00 & LN_AND;

				shade = (LN_AND - frac) >> 4;
				*(vram + vramofs) |= activeGradient[shade];

				shade = frac >> 4;
				*(vram + vramofs + 1) |= activeGradient[shade];
			}
            x00+=l;
		}
	}
}

static void prepareEdgeListGouraud(ScreenElement *e0, ScreenElement *e1)
{
	Edge *edgeListToWrite;
	ScreenElement *eTemp;

	if (e0->y == e1->y) return;

	// Assumes CCW
	if (e0->y < e1->y) {
		edgeListToWrite = leftEdge;
	}
	else {
		edgeListToWrite = rightEdge;

		eTemp = e0;
		e0 = e1;
		e1 = eTemp;
	}

    {
        const int x0 = e0->x; int y0 = e0->y; int c0 = e0->c;
        const int x1 = e1->x; int y1 = e1->y; int c1 = e1->c;

        int dy = y1 - y0 + 1;
		const int repDiv = divTab[dy + DIV_TAB_SIZE / 2];
        const int dx = ((x1 - x0) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE);
		const int dc = ((c1 - c0) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE);

        int fx = INT_TO_FIXED(x0, FP_BASE);
		int fc = INT_TO_FIXED(c0, FP_BASE);

		if (y0 < 0) {
			fx += -y0 * dx;
			fc += -y0 * dc;
			dy += y0;
			y0 = 0;
		}
		if (y1 > SOFT_BUFF_HEIGHT-1) {
			dy -= (y1 - SOFT_BUFF_HEIGHT-1);
		}

        edgeListToWrite = &edgeListToWrite[y0];
        do {
			int x = FIXED_TO_INT(fx, FP_BASE);
			CLAMP(x, 0, SOFT_BUFF_WIDTH-1)
			edgeListToWrite->x = x;
			edgeListToWrite->c = fc;
            ++edgeListToWrite;
            fx += dx;
			fc += dc;
		} while(--dy > 0);
    }
}

static void prepareEdgeListEnvmap(ScreenElement *e0, ScreenElement *e1)
{
	Edge *edgeListToWrite;
	ScreenElement *eTemp;

	if (e0->y == e1->y) return;

	// Assumes CCW
	if (e0->y < e1->y) {
		edgeListToWrite = leftEdge;
	}
	else {
		edgeListToWrite = rightEdge;

		eTemp = e0;
		e0 = e1;
		e1 = eTemp;
	}

    {
        const int x0 = e0->x; int y0 = e0->y; int u0 = e0->u; int v0 = e0->v;
        const int x1 = e1->x; int y1 = e1->y; int u1 = e1->u; int v1 = e1->v;

        int dy = y1 - y0 + 1;
		const int repDiv = divTab[dy + DIV_TAB_SIZE / 2];
        const int dx = ((x1 - x0) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE);
		const int du = ((u1 - u0) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE);
		const int dv = ((v1 - v0) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE);

        int fx = INT_TO_FIXED(x0, FP_BASE);
		int fu = INT_TO_FIXED(u0, FP_BASE);
		int fv = INT_TO_FIXED(v0, FP_BASE);

		if (y0 < 0) {
			fx += -y0 * dx;
			fu += -y0 * du;
			fv += -y0 * dv;
			dy += y0;
			y0 = 0;
		}
		if (y1 > SOFT_BUFF_HEIGHT-1) {
			dy -= (y1 - SOFT_BUFF_HEIGHT-1);
		}

        edgeListToWrite = &edgeListToWrite[y0];
        do {
			int x = FIXED_TO_INT(fx, FP_BASE);
			CLAMP(x, 0, SOFT_BUFF_WIDTH-1)
			edgeListToWrite->x = x;
			edgeListToWrite->u = fu;
			edgeListToWrite->v = fv;
            ++edgeListToWrite;
            fx += dx;
			fu += du;
			fv += dv;
		} while(--dy > 0);
    }
}

static void prepareEdgeListGouraudEnvmap(ScreenElement *e0, ScreenElement *e1)
{
	Edge *edgeListToWrite;
	ScreenElement *eTemp;

	if (e0->y == e1->y) return;

	// Assumes CCW
	if (e0->y < e1->y) {
		edgeListToWrite = leftEdge;
	}
	else {
		edgeListToWrite = rightEdge;

		eTemp = e0;
		e0 = e1;
		e1 = eTemp;
	}

    {
        const int x0 = e0->x; int y0 = e0->y; int c0 = e0->c; int u0 = e0->u; int v0 = e0->v;
        const int x1 = e1->x; int y1 = e1->y; int c1 = e1->c; int u1 = e1->u; int v1 = e1->v;

        int dy = y1 - y0 + 1;
		const int repDiv = divTab[dy + DIV_TAB_SIZE / 2];
        const int dx = ((x1 - x0) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE);
		const int dc = ((c1 - c0) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE);
		const int du = ((u1 - u0) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE);
		const int dv = ((v1 - v0) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE);

        int fx = INT_TO_FIXED(x0, FP_BASE);
		int fc = INT_TO_FIXED(c0, FP_BASE);
		int fu = INT_TO_FIXED(u0, FP_BASE);
		int fv = INT_TO_FIXED(v0, FP_BASE);

		if (y0 < 0) {
			fx += -y0 * dx;
			fc += -y0 * dc;
			fu += -y0 * du;
			fv += -y0 * dv;
			dy += y0;
			y0 = 0;
		}
		if (y1 > SOFT_BUFF_HEIGHT-1) {
			dy -= (y1 - SOFT_BUFF_HEIGHT-1);
		}

        edgeListToWrite = &edgeListToWrite[y0];
        do {
			int x = FIXED_TO_INT(fx, FP_BASE);
			CLAMP(x, 0, SOFT_BUFF_WIDTH-1)
			edgeListToWrite->x = x;
			edgeListToWrite->c = fc;
			edgeListToWrite->u = fu;
			edgeListToWrite->v = fv;
            ++edgeListToWrite;
            fx += dx;
			fc += dc;
			fu += du;
			fv += dv;
		} while(--dy > 0);
    }
}

static void fillGouraudEdges8_SemiSoft(int yMin, int yMax)
{
	Edge *le = &leftEdge[yMin];
	Edge *re = &rightEdge[yMin];

	int y;
	CCB *firstCel = *currentScanlineCel8;

	for (y=yMin; y<=yMax; ++y) {
		const int xl = le->x;
		int cl = le->c;
		int cr = re->c;
		int length = re->x - xl;

		CCB *cel = *currentScanlineCel8++;

		cl = FIXED_TO_INT(cl, FP_BASE);
		cr = FIXED_TO_INT(cr, FP_BASE);
		CLAMP(cl,0,GRADIENT_LENGTH)
		CLAMP(cr,0,GRADIENT_LENGTH)

		cel->ccb_Flags &= ~CCB_LDPLUT;

		cel->ccb_XPos = xl<<16;
		cel->ccb_YPos = y<<16;

		cel->ccb_HDX = (length<<20) / GRADIENT_LENGTH;
		
		cel->ccb_SourcePtr = (CelData*)&gourGrads[cl*GRADIENT_GROUP_SIZE + cr*GRADIENT_LENGTH];

		++le;
		++re;
	}
	firstCel->ccb_PLUTPtr = (void*)activeGradient;
	firstCel->ccb_Flags |= CCB_LDPLUT;
}

static void fillGouraudEdges8(int yMin, int yMax)
{
	uint8 *vram8 = softBuffer8 + yMin * SOFT_BUFF_WIDTH;
	int count = yMax - yMin + 1;
	Edge *le = &leftEdge[yMin];
	Edge *re = &rightEdge[yMin];
	do {
		const int xl = le->x;
		const int cl = le->c;
		const int cr = re->c;
		int length = re->x - xl;
		uint8 *dst = vram8 + xl;
		uint32 *dst32;

		const int repDiv = divTab[length + DIV_TAB_SIZE / 2];
		const int dc = (((cr - cl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		int fc = cl;

		int xlp = xl & 3;
		if (xlp) {
			xlp = 4 - xlp;
			while (xlp-- > 0 && length-- > 0) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				CLAMP(c, 1, COLOR_GRADIENTS_SIZE-1)
				fc += dc;

				*dst++ = c;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c0 = FIXED_TO_INT(fc, FP_BASE);
			CLAMP_LEFT(c0, 1)
			fc += dc;
			c1 = FIXED_TO_INT(fc, FP_BASE);
			CLAMP_LEFT(c1, 1)
			fc += dc;
			c2 = FIXED_TO_INT(fc, FP_BASE);
			CLAMP_LEFT(c2, 1)
			fc += dc;
			c3 = FIXED_TO_INT(fc, FP_BASE);
			CLAMP_LEFT(c3, 1)
			fc += dc;

			*dst32++ = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
			length-=4;
		};

		dst = (uint8*)dst32;
		while (length-- > 0) {
			int c = FIXED_TO_INT(fc, FP_BASE);
			CLAMP(c, 1, COLOR_GRADIENTS_SIZE-1)
			fc += dc;

			*dst++ = c;
		}

		++le;
		++re;
		vram8 += SOFT_BUFF_WIDTH;
	} while(--count > 0);
}

static void fillGouraudEdges16(int yMin, int yMax)
{
	uint16 *vram16 = softBuffer16 + yMin * SOFT_BUFF_WIDTH;
	int count = yMax - yMin + 1;
	Edge *le = &leftEdge[yMin];
	Edge *re = &rightEdge[yMin];
	do {
		const int xl = le->x;
		const int cl = le->c;
		const int cr = re->c;
		int length = re->x - xl;
		uint16 *dst = vram16 + xl;
		uint32 *dst32;

		const int repDiv = divTab[length + DIV_TAB_SIZE / 2];
		const int dc = (((cr - cl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		int fc = cl;

		if (length>0){
			if (xl & 1) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				CLAMP(c, 0, COLOR_GRADIENTS_SIZE-1)
				fc += dc;

				*dst++ = activeGradient[c];
				length--;
			}

			dst32 = (uint32*)dst;
			while(length >= 2) {
				int c0, c1;

				c0 = FIXED_TO_INT(fc, FP_BASE);
				//CLAMP(c0, 0, COLOR_GRADIENTS_SIZE-1)
				fc += dc;
				c1 = FIXED_TO_INT(fc, FP_BASE);
				//CLAMP(c1, 0, COLOR_GRADIENTS_SIZE-1)
				fc += dc;

				*dst32++ = (activeGradient[c0] << 16) | activeGradient[c1];
				length -= 2;
			};

			dst = (uint16*)dst32;
			if (length & 1) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				CLAMP(c, 0, COLOR_GRADIENTS_SIZE-1)
				fc += dc;

				*dst++ = activeGradient[c];
			}
		}

		++le;
		++re;
		vram16 += SOFT_BUFF_WIDTH;
	} while(--count > 0);
}

static void fillEnvmapEdges8(int yMin, int yMax)
{
	uint8 *vram8 = softBuffer8 + yMin * SOFT_BUFF_WIDTH;
	int count = yMax - yMin + 1;
	Edge *le = &leftEdge[yMin];
	Edge *re = &rightEdge[yMin];

	const int texWidth = activeTexture->width;
	const int texHeight = activeTexture->height;
	uint8* texData = (uint8*)activeTexture->bitmap;

	do {
		const int xl = le->x;
		const int ul = le->u;
		const int ur = re->u;
		const int vl = le->v;
		const int vr = re->v;
		int length = re->x - xl;
		uint8 *dst = vram8 + xl;
		uint32 *dst32;

		const int repDiv = divTab[length + DIV_TAB_SIZE / 2];
		const int du = (((ur - ul) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		const int dv = (((vr - vl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		int fu = ul;
		int fv = vl;

		int u, v;

		int xlp = xl & 3;
		if (xlp) {
			xlp = 4 - xlp;
			while (xlp-- > 0 && length-- > 0) {
				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				fu += du;
				fv += dv;

				*dst++ = texData[v * texWidth + u];
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			c0 = texData[v * texWidth + u];
			fu += du;
			fv += dv;

			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			c1 = texData[v * texWidth + u];
			fu += du;
			fv += dv;

			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			c2 = texData[v * texWidth + u];
			fu += du;
			fv += dv;

			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			c3 = texData[v * texWidth + u];
			fu += du;
			fv += dv;

			*dst32++ = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
			length-=4;
		};

		dst = (uint8*)dst32;
		while (length-- > 0) {
			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			fu += du;
			fv += dv;

			*dst++ = texData[v * texWidth + u];
		}

		++le;
		++re;
		vram8 += SOFT_BUFF_WIDTH;
	} while(--count > 0);
}

static void fillEnvmapEdges16(int yMin, int yMax)
{
	uint16 *vram16 = softBuffer16 + yMin * SOFT_BUFF_WIDTH;
	int count = yMax - yMin + 1;
	Edge *le = &leftEdge[yMin];
	Edge *re = &rightEdge[yMin];

	const int texWidth = activeTexture->width;
	const int texHeight = activeTexture->height;
	uint16* texData = (uint16*)activeTexture->bitmap;

	do {
		const int xl = le->x;
		const int ul = le->u;
		const int ur = re->u;
		const int vl = le->v;
		const int vr = re->v;
		int length = re->x - xl;
		uint16 *dst = vram16 + xl;
		uint32 *dst32;

		const int repDiv = divTab[length + DIV_TAB_SIZE / 2];
		const int du = (((ur - ul) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		const int dv = (((vr - vl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		int fu = ul;
		int fv = vl;

		if (length>0){
			int u, v;
			if (xl & 1) {
				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				fu += du;
				fv += dv;

				*dst++ = texData[v * texWidth + u];
				length--;
			}

			dst32 = (uint32*)dst;
			while(length >= 2) {
				int c0, c1;

				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				c0 = texData[v * texWidth + u];
				fu += du;
				fv += dv;

				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				c1 = texData[v * texWidth + u];
				fu += du;
				fv += dv;

				*dst32++ = (c0 << 16) | c1;

				length -= 2;
			};

			dst = (uint16*)dst32;
			if (length & 1) {
				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				fu += du;
				fv += dv;

				*dst++ = texData[v * texWidth + u];
			}
		}

		++le;
		++re;
		vram16 += SOFT_BUFF_WIDTH;
	} while(--count > 0);
}

static void fillGouraudEnvmapEdges8(int yMin, int yMax)
{
	uint8 *vram8 = softBuffer8 + yMin * SOFT_BUFF_WIDTH;
	int count = yMax - yMin + 1;
	Edge *le = &leftEdge[yMin];
	Edge *re = &rightEdge[yMin];

	const int texWidth = activeTexture->width;
	const int texHeight = activeTexture->height;
	uint8* texData = (uint8*)activeTexture->bitmap;

	do {
		const int xl = le->x;
		const int cl = le->c;
		const int cr = re->c;
		const int ul = le->u;
		const int ur = re->u;
		const int vl = le->v;
		const int vr = re->v;
		int length = re->x - xl;
		uint8 *dst = vram8 + xl;
		uint32 *dst32;

		const int repDiv = divTab[length + DIV_TAB_SIZE / 2];
		const int dc = (((cr - cl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		const int du = (((ur - ul) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		const int dv = (((vr - vl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		int fc = cl;
		int fu = ul;
		int fv = vl;

		int u, v, c;

		int xlp = xl & 3;
		if (xlp) {
			xlp = 4 - xlp;
			while (xlp-- > 0 && length-- > 0) {
				c = FIXED_TO_INT(fc, FP_BASE);
				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				fc += dc;
				fu += du;
				fv += dv;

				c = (texData[v * texWidth + u] * c) >> COLOR_ENVMAP_SHR;
				CLAMP(c, 1, COLOR_GRADIENTS_SIZE-1)
				*dst++ = c;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c = FIXED_TO_INT(fc, FP_BASE);
			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			c0 = (texData[v * texWidth + u] * c) >> COLOR_ENVMAP_SHR;
			CLAMP(c0, 1, COLOR_GRADIENTS_SIZE-1)
			fc += dc;
			fu += du;
			fv += dv;

			c = FIXED_TO_INT(fc, FP_BASE);
			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			c1 = (texData[v * texWidth + u] * c) >> COLOR_ENVMAP_SHR;
			CLAMP(c1, 1, COLOR_GRADIENTS_SIZE-1)
			fc += dc;
			fu += du;
			fv += dv;

			c = FIXED_TO_INT(fc, FP_BASE);
			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			c2 = (texData[v * texWidth + u] * c) >> COLOR_ENVMAP_SHR;
			CLAMP(c2, 1, COLOR_GRADIENTS_SIZE-1)
			fc += dc;
			fu += du;
			fv += dv;

			c = FIXED_TO_INT(fc, FP_BASE);
			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			c3 = (texData[v * texWidth + u] * c) >> COLOR_ENVMAP_SHR;
			CLAMP(c3, 1, COLOR_GRADIENTS_SIZE-1)
			fc += dc;
			fu += du;
			fv += dv;

			*dst32++ = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
			length-=4;
		};

		dst = (uint8*)dst32;
		while (length-- > 0) {
			c = FIXED_TO_INT(fc, FP_BASE);
			u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
			v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
			fc += dc;
			fu += du;
			fv += dv;

			c = (texData[v * texWidth + u] * c) >> COLOR_ENVMAP_SHR;
			CLAMP(c, 1, COLOR_GRADIENTS_SIZE-1)
			*dst++ = c;
		}

		++le;
		++re;
		vram8 += SOFT_BUFF_WIDTH;
	} while(--count > 0);
}

static void fillGouraudEnvmapEdges16(int yMin, int yMax)
{
	uint16 *vram16 = softBuffer16 + yMin * SOFT_BUFF_WIDTH;
	int count = yMax - yMin + 1;
	Edge *le = &leftEdge[yMin];
	Edge *re = &rightEdge[yMin];

	const int texWidth = activeTexture->width;
	const int texHeight = activeTexture->height;
	uint16* texData = (uint16*)activeTexture->bitmap;

	do {
		const int xl = le->x;
		const int cl = le->c;
		const int cr = re->c;
		const int ul = le->u;
		const int ur = re->u;
		const int vl = le->v;
		const int vr = re->v;
		int length = re->x - xl;
		uint16 *dst = vram16 + xl;
		uint32 *dst32;

		const int repDiv = divTab[length + DIV_TAB_SIZE / 2];
		const int dc = (((cr - cl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		const int du = (((ur - ul) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		const int dv = (((vr - vl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		int fc = cl;
		int fu = ul;
		int fv = vl;

		if (length>0){
			int u, v, c;
			if (xl & 1) {
				c = FIXED_TO_INT(fc, FP_BASE);
				CLAMP(c, 1, COLOR_GRADIENTS_SIZE-1)
				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				fc += dc;
				fu += du;
				fv += dv;

				*dst++ = texData[v * texWidth + u];
				length--;
			}

			dst32 = (uint32*)dst;
			while(length >= 2) {
				int c0, c1;

				c = FIXED_TO_INT(fc, FP_BASE);
				CLAMP(c, 1, COLOR_GRADIENTS_SIZE-1)
				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				c0 = texData[v * texWidth + u];
				fc += dc;
				fu += du;
				fv += dv;

				c = FIXED_TO_INT(fc, FP_BASE);
				CLAMP(c, 1, COLOR_GRADIENTS_SIZE-1)
				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				c1 = texData[v * texWidth + u];
				fc += dc;
				fu += du;
				fv += dv;

				*dst32++ = (c0 << 16) | c1;

				length -= 2;
			};

			dst = (uint16*)dst32;
			if (length & 1) {
				c = FIXED_TO_INT(fc, FP_BASE);
				u = (FIXED_TO_INT(fu, FP_BASE)) & (texWidth-1);
				v = (FIXED_TO_INT(fv, FP_BASE)) & (texHeight-1);
				fc += dc;
				fu += du;
				fv += dv;

				*dst++ = texData[v * texWidth + u];
			}
		}

		++le;
		++re;
		vram16 += SOFT_BUFF_WIDTH;
	} while(--count > 0);
}

static void drawTriangle(ScreenElement *e0, ScreenElement *e1, ScreenElement *e2)
{
	int yMin = e0->y;
	int yMax = yMin;

	prepareEdgeList(e0, e1);
	prepareEdgeList(e1, e2);
	prepareEdgeList(e2, e0);

	if (e1->y < yMin) yMin = e1->y; if (e1->y > yMax) yMax = e1->y;
	if (e2->y < yMin) yMin = e2->y; if (e2->y > yMax) yMax = e2->y;

	if (yMin < 0) yMin = 0;
	if (yMax > SOFT_BUFF_HEIGHT-1) yMax = SOFT_BUFF_HEIGHT-1;

	fillEdges(yMin, yMax);
}


static void clearSoftBuffer()
{
	vramSet(0, (void*)getSpriteBitmapData(sprSoftBuffer), getCelDataSizeInBytes(sprSoftBuffer->cel));
}

static void prepareMeshSoftRender(Mesh *ms)
{
	if (ms->renderType & MESH_OPTION_RENDER_SEMISOFT) {
		currentScanlineCel8 = scanlineCel8;
	} else {
		clearSoftBuffer();
	}

	switch(renderSoftMethod) {
		case RENDER_SOFT_METHOD_GOURAUD:
		{
			prepareEdgeList = prepareEdgeListGouraud;
			fillEdges = fillGouraudEdges16;
			if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
				fillEdges = fillGouraudEdges8;
			} else if (ms->renderType & MESH_OPTION_RENDER_SEMISOFT) {
				fillEdges = fillGouraudEdges8_SemiSoft;
			}
		}
		break;

		case RENDER_SOFT_METHOD_ENVMAP:
		{
			prepareEdgeList = prepareEdgeListEnvmap;
			fillEdges = fillEnvmapEdges16;
			if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
				fillEdges = fillEnvmapEdges8;
			}
		}
		break;

		case RENDER_SOFT_METHOD_GOURAUD_ENVMAP:
		{
			prepareEdgeList = prepareEdgeListGouraudEnvmap;
			fillEdges = fillGouraudEnvmapEdges16;
			if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
				fillEdges = fillGouraudEnvmapEdges8;
			}
		}
		break;

		default:
		break;
	}
}

static void bindMeshPolyData(Mesh *ms, int numPoly)
{
	if (renderSoftMethod <= RENDER_SOFT_METHOD_GOURAUD) {
		bindGradient(lineColorShades[numPoly & 3]);
	} else {
		bindTexture(&ms->tex[ms->poly[numPoly].textureId]);
	}
}

static void finalizeMeshSoftRender(Mesh *ms)
{
	if (ms->renderType & MESH_OPTION_RENDER_SEMISOFT) {
		CCB *lastScanlineCel = *(currentScanlineCel8 - 1);
		lastScanlineCel->ccb_Flags |= CCB_LAST;
		drawCels(*scanlineCel8);
		lastScanlineCel->ccb_Flags &= ~CCB_LAST;
	} else {
		drawSprite(sprSoftBuffer);
	}
}


static void renderMeshSoft(Mesh *ms, ScreenElement *elements)
{
	ScreenElement *e0, *e1, *e2;
	int i,n;

	int *index = ms->index;

	prepareMeshSoftRender(ms);

	for (i=0; i<ms->polysNum; ++i) {
		e0 = &elements[*index++];
		e1 = &elements[*index++];
		e2 = &elements[*index++];

		n = (e0->x - e1->x) * (e2->y - e1->y) - (e2->x - e1->x) * (e0->y - e1->y);
		if (n > 0) {
			bindMeshPolyData(ms, i);
			drawTriangle(e0, e1, e2);

			if (ms->poly[i].numPoints == 4) {	// if quad then render another triangle
				e1 = e2;
				e2 = &elements[*index];
				drawTriangle(e0, e1, e2);
			}
		}
		if (ms->poly[i].numPoints == 4) ++index;
	}

	finalizeMeshSoftRender(ms);
}

static void renderMeshSoftWireframe(Mesh *ms, ScreenElement *elements)
{
	ScreenElement *e0, *e1;

	int *lineIndex = ms->lineIndex;
	int i;

	prepareMeshSoftRender(ms);

	for (i=0; i<ms->linesNum; ++i) {
		e0 = &elements[*lineIndex++];
		e1 = &elements[*lineIndex++];

		bindMeshPolyData(ms, i);
		drawAntialiasedLine(e0, e1);
	}

	finalizeMeshSoftRender(ms);
}

void renderTransformedMeshSoft(Mesh *ms, ScreenElement *elements)
{
	sprSoftBuffer = sprSoftBuffer16;
	if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
		sprSoftBuffer = sprSoftBuffer8;
	}

	if (renderSoftMethod == RENDER_SOFT_METHOD_WIREFRAME) {
		renderMeshSoftWireframe(ms, elements);
	} else {
		renderMeshSoft(ms, elements);
	}
}

void setRenderSoftMethod(int method)
{
	renderSoftMethod = method;
}


void initEngineSoft()
{
	if (!sprSoftBuffer8) sprSoftBuffer8 = newSprite(SOFT_BUFF_WIDTH, SOFT_BUFF_HEIGHT, 8, CEL_TYPE_CODED, NULL, (void*)softBuffer8);
	if (!sprSoftBuffer16) sprSoftBuffer16 = newSprite(SOFT_BUFF_WIDTH, SOFT_BUFF_HEIGHT, 16, CEL_TYPE_UNCODED, NULL, (void*)softBuffer16);

	initDivs();
	initSemiSoftGouraud();

	if (!lineColorShades[0]) lineColorShades[0] = crateColorShades(31,23,15, COLOR_GRADIENTS_SIZE, false);
	if (!lineColorShades[1]) lineColorShades[1] = crateColorShades(15,23,31, COLOR_GRADIENTS_SIZE, false);
	if (!lineColorShades[2]) lineColorShades[2] = crateColorShades(15,31,23, COLOR_GRADIENTS_SIZE, false);
	if (!lineColorShades[3]) lineColorShades[3] = crateColorShades(31,15,23, COLOR_GRADIENTS_SIZE, false);

	if (!gouraudColorShades) gouraudColorShades = crateColorShades(27,29,31, COLOR_GRADIENTS_SIZE, true);
	sprSoftBuffer8->cel->ccb_PLUTPtr = gouraudColorShades;
}
