#include "core.h"

#include "tools.h"

#include "engine_main.h"
#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"

#include "system_graphics.h"

#include "cel_helpers.h"
#include "mathutil.h"
#include "tools.h"


#define OLD_TRIANGLE_DRAW

#define SOFT_BUFF_MAX_SIZE (2 * SCREEN_WIDTH * SCREEN_HEIGHT)

#define DIV_TAB_SIZE 4096
#define DIV_TAB_SHIFT 12

// Semisoft gouraud method
#define MAX_SCANLINES 4096
static CCB **scanlineCel8;
static CCB **currentScanlineCel8;

#define GRADIENT_SHADES 32
#define GRADIENT_LENGTH GRADIENT_SHADES
#define GRADIENT_GROUP_SIZE (GRADIENT_SHADES * GRADIENT_LENGTH)
static unsigned char *gourGradBmps;

static bool fastGouraud = false;

typedef struct Edge
{
	int x;
	int c;
	int u,v;
}Edge;

typedef struct Gradients
{
	int dx,dc,du,dv;
}Gradients;

typedef struct SoftBuffer
{
	int bpp;
	int width;
	int height;
	int stride;
	int currentIndex;
	unsigned char *data;
	CCB *cel;
}SoftBuffer;

static int renderSoftMethod = RENDER_SOFT_METHOD_GOURAUD;

SoftBuffer softBuffer;
void *softBufferCurrentPtr;

static Edge *leftEdge;
static Edge *rightEdge;
static int32 *divTab;
static Gradients grads;

static uint16 *lineColorShades[4] = { NULL, NULL, NULL, NULL };
static uint16 *gouraudColorShades;

static uint16 *activeGradient = NULL;
static Texture *activeTexture = NULL;

static int minX, maxX, minY, maxY;

#define LN_BASE 8
#define LN_AND ((1 << LN_BASE) - 1)


static void(*fillEdges)(int y0, int y1);
static void(*prepareEdgeList) (ScreenElement *e0, ScreenElement *e1);

static void(*fillTriangleOld) (int y0, int y1, ScreenElement *e0, ScreenElement *e1, Gradients *slope1, Gradients *slope2);


static void bindGradient(uint16 *gradient)
{
	activeGradient = gradient;
}

static void bindTexture(Texture *texture)
{
	activeTexture = texture;
}

static void bindMeshPolyData(Mesh *ms, int numPoly)
{
	if (renderSoftMethod <= RENDER_SOFT_METHOD_GOURAUD) {
		bindGradient(lineColorShades[numPoly & 3]);
	} else {
		bindTexture(&ms->tex[ms->poly[numPoly].textureId]);
	}
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
	unsigned char *dst;
	int c0,c1,x;

	gourGradBmps = (unsigned char*)AllocMem(GRADIENT_SHADES * GRADIENT_GROUP_SIZE, MEMTYPE_ANY);
	scanlineCel8 = (CCB**)AllocMem(MAX_SCANLINES * sizeof(CCB*), MEMTYPE_ANY);
	currentScanlineCel8 = scanlineCel8;

	dst = gourGradBmps;
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
			const int dc = ((c1 - c0) << FP_BASE) / GRADIENT_LENGTH;
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

	uint16 *vram = (uint16*)softBufferCurrentPtr;
	const int screenWidth = softBuffer.width;
	const int screenHeight = softBuffer.height;
	const int stride16 = softBuffer.stride >> 1;

    // ==== Clipping ====

    int outcode1 = 0, outcode2 = 0;

    if (y1 < 0) outcode1 |= 0x0001;
        else if (y1 > screenHeight-1) outcode1 |= 0x0010;
    if (x1 < 0) outcode1 |= 0x0100;
        else if (x1 > screenWidth-1) outcode1 |= 0x1000;

    if (y2 < 0) outcode2 |= 0x0001;
        else if (y2 > screenHeight-1) outcode2 |= 0x0010;
    if (x2 < 0) outcode2 |= 0x0100;
        else if (x2 > screenWidth-1) outcode2 |= 0x1000;

    //if ((outcode1 & outcode2)!=0) return;

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

			if (x >= 0 && x < screenWidth && yp >=0 && yp < screenHeight - 1) {
				vramofs = yp*stride16 + x;
				frac = y00 & LN_AND;

				shade = (LN_AND - frac) >> 4;
				*(vram + vramofs) |= activeGradient[shade];

				shade = frac >> 4;
				*(vram + vramofs+stride16) |= activeGradient[shade];
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

			if (y >= 0 && y < screenHeight && xp >=0 && xp < screenWidth - 1) {
				vramofs = y*stride16 + xp;
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

static void calculateTriangleGradients(ScreenElement *e0, ScreenElement *e1, ScreenElement *e2)
{
	const int x0 = e0->x; const int x1 = e1->x; const int x2 = e2->x;
	const int y0 = e0->y; const int y1 = e1->y; const int y2 = e2->y;

	if (renderSoftMethod & RENDER_SOFT_METHOD_GOURAUD) {
		const int c0 = e0->c; const int c1 = e1->c; const int c2 = e2->c;
		grads.dc = (((c1-c2)*(y0-y2) - (c0-c2)*(y1-y2)) << FP_BASE) / ((x1-x2)*(y0-y2) - (x0-x2)*(y1-y2));
	}

	if (renderSoftMethod & RENDER_SOFT_METHOD_ENVMAP) {
		const int u0 = e0->u; const int u1 = e1->u; const int u2 = e2->u;
		const int v0 = e0->v; const int v1 = e1->v; const int v2 = e2->v;
		grads.du = (((u1-u2)*(y0-y2) - (u0-u2)*(y1-y2)) << FP_BASE) / ((x1-x2)*(y0-y2) - (x0-x2)*(y1-y2));
		grads.dv = (((v1-v2)*(y0-y2) - (v0-v2)*(y1-y2)) << FP_BASE) / ((x1-x2)*(y0-y2) - (x0-x2)*(y1-y2));
	}
}

static void prepareEdgeListGouraud(ScreenElement *e0, ScreenElement *e1)
{
	Edge *edgeListToWrite;
	ScreenElement *eTemp;
	int32 *dvt = &divTab[DIV_TAB_SIZE/2];

	//if (e0->y == e1->y) return;

	// Assumes CCW
	if (e0->y < e1->y) {
		edgeListToWrite = leftEdge;

		{
			const int x0 = e0->x; int y0 = e0->y; int c0 = e0->c;
			const int x1 = e1->x; int y1 = e1->y; int c1 = e1->c;

			int dy = y1 - y0;
			const int repDiv = dvt[dy];
			const int dx = ((x1 - x0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
			const int dc = ((c1 - c0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);

			int fx = INT_TO_FIXED(x0, FP_BASE);
			int fc = INT_TO_FIXED(c0, FP_BASE);

			edgeListToWrite = &edgeListToWrite[y0];
			while(dy-- > 0) {
				int x = FIXED_TO_INT(fx, FP_BASE);
				edgeListToWrite->x = x;
				edgeListToWrite->c = fc;
				++edgeListToWrite;
				fx += dx;
				fc += dc;
			};
		}
	}
	else {
		edgeListToWrite = rightEdge;

		eTemp = e0;
		e0 = e1;
		e1 = eTemp;

		{
			const int x0 = e0->x; int y0 = e0->y;
			const int x1 = e1->x; int y1 = e1->y;

			int dy = y1 - y0;
			const int repDiv = dvt[dy];
			const int dx = ((x1 - x0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);

			int fx = INT_TO_FIXED(x0, FP_BASE);

			edgeListToWrite = &edgeListToWrite[y0];
			while(dy-- > 0) {
				int x = FIXED_TO_INT(fx, FP_BASE);
				edgeListToWrite->x = x;
				++edgeListToWrite;
				fx += dx;
			};
		}
	}
}

static void prepareEdgeListEnvmap(ScreenElement *e0, ScreenElement *e1)
{
	Edge *edgeListToWrite;
	ScreenElement *eTemp;
	int32 *dvt = &divTab[DIV_TAB_SIZE/2];

	//if (e0->y == e1->y) return;

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

        int dy = y1 - y0;
		const int repDiv = dvt[dy];
        const int dx = ((x1 - x0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
		const int du = ((u1 - u0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
		const int dv = ((v1 - v0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);

        int fx = INT_TO_FIXED(x0, FP_BASE);
		int fu = INT_TO_FIXED(u0, FP_BASE);
		int fv = INT_TO_FIXED(v0, FP_BASE);

		/*if (y0 < 0) {
			fx += -y0 * dx;
			fu += -y0 * du;
			fv += -y0 * dv;
			dy += y0;
			y0 = 0;
		}
		if (y1 > SCREEN_HEIGHT-1) {
			dy -= (y1 - SCREEN_HEIGHT-1);
		}*/

        edgeListToWrite = &edgeListToWrite[y0];
        while(dy-- > 0) {
			int x = FIXED_TO_INT(fx, FP_BASE);
			edgeListToWrite->x = x;
			edgeListToWrite->u = fu;
			edgeListToWrite->v = fv;
            ++edgeListToWrite;
            fx += dx;
			fu += du;
			fv += dv;
		};
    }
}

static void prepareEdgeListGouraudEnvmap(ScreenElement *e0, ScreenElement *e1)
{
	Edge *edgeListToWrite;
	ScreenElement *eTemp;
	int32 *dvt = &divTab[DIV_TAB_SIZE/2];

	//if (e0->y == e1->y) return;

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

        int dy = y1 - y0;
		const int repDiv = dvt[dy];
        const int dx = ((x1 - x0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
		const int dc = ((c1 - c0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
		const int du = ((u1 - u0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);
		const int dv = ((v1 - v0) * repDiv) >> (DIV_TAB_SHIFT - FP_BASE);

        int fx = INT_TO_FIXED(x0, FP_BASE);
		int fc = INT_TO_FIXED(c0, FP_BASE);
		int fu = INT_TO_FIXED(u0, FP_BASE);
		int fv = INT_TO_FIXED(v0, FP_BASE);

		/*if (y0 < 0) {
			fx += -y0 * dx;
			fc += -y0 * dc;
			fu += -y0 * du;
			fv += -y0 * dv;
			dy += y0;
			y0 = 0;
		}
		if (y1 > SCREEN_HEIGHT-1) {
			dy -= (y1 - SCREEN_HEIGHT-1);
		}*/

        edgeListToWrite = &edgeListToWrite[y0];
        while(dy-- > 0) {
			int x = FIXED_TO_INT(fx, FP_BASE);
			//CLAMP(x, 0, SCREEN_WIDTH-1)
			edgeListToWrite->x = x;
			edgeListToWrite->c = fc;
			edgeListToWrite->u = fu;
			edgeListToWrite->v = fv;
            ++edgeListToWrite;
            fx += dx;
			fc += dc;
			fu += du;
			fv += dv;
		};
    }
}

static void fillGouraudEdges8_SemiSoft(int y0, int y1)
{
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	int y;
	CCB *firstCel = *currentScanlineCel8;

	for (y=y0; y<=y1; ++y) {
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
		
		cel->ccb_SourcePtr = (CelData*)&gourGradBmps[cl*GRADIENT_GROUP_SIZE + cr*GRADIENT_LENGTH];

		++le;
		++re;
	}
	firstCel->ccb_PLUTPtr = (void*)activeGradient;
	firstCel->ccb_Flags |= CCB_LDPLUT;
}

static void fillGouraudEdges8(int y0, int y1)
{
	const int stride8 = softBuffer.stride;
	unsigned char *vram8 = (unsigned char*)softBufferCurrentPtr + y0 * stride8;

	const int dc = grads.dc;

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];
	do {
		const int xl = le->x;
		const int cl = le->c;
		int length = re->x - xl;
		unsigned char *dst = vram8 + xl;
		uint32 *dst32;

		int fc = cl;

		int xlp = xl & 3;
		if (xlp) {
			xlp = 4 - xlp;
			while (xlp-- > 0 && length-- > 0) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;

				*dst++ = c;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c0 = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;
			c1 = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;
			c2 = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;
			c3 = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;

			#ifdef BIG_ENDIAN
				*dst32++ = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
			#else
				*dst32++ = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
			#endif
			length-=4;
		};

		dst = (unsigned char*)dst32;
		while (length-- > 0) {
			int c = FIXED_TO_INT(fc, FP_BASE);
			fc += dc;

			*dst++ = c;
		}

		++le;
		++re;
		vram8 += stride8;
	} while(--count > 0);
}


static void fillGouraudEdges16(int y0, int y1)
{
	const int stride16 = softBuffer.stride >> 1;
	uint16 *vram16 = (uint16*)softBufferCurrentPtr + y0 * stride16;

	const int dc = grads.dc;

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];
	do {
		const int xl = le->x;
		const int cl = le->c;
		int length = re->x - xl;
		uint16 *dst = vram16 + xl;
		uint32 *dst32;

		int fc = cl;

		if (length>0){
			if (xl & 1) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;

				*dst++ = activeGradient[c];
				length--;
			}

			dst32 = (uint32*)dst;
			while(length >= 2) {
				int c0, c1;

				c0 = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;
				c1 = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;

				#ifdef BIG_ENDIAN
					*dst32++ = (activeGradient[c0] << 16) | activeGradient[c1];
				#else
					*dst32++ = (activeGradient[c1] << 16) | activeGradient[c0];
				#endif
				length -= 2;
			};

			dst = (uint16*)dst32;
			if (length & 1) {
				int c = FIXED_TO_INT(fc, FP_BASE);
				fc += dc;

				*dst++ = activeGradient[c];
			}
		}

		++le;
		++re;
		vram16 += stride16;
	} while(--count > 0);
}

static void fillEnvmapEdges8(int y0, int y1)
{
	const int stride8 = softBuffer.stride;
	unsigned char *vram8 = (unsigned char*)softBufferCurrentPtr + y0 * stride8;
	int32 *dvt = &divTab[DIV_TAB_SIZE/2];

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	const int texHeightShift = activeTexture->hShift;
	unsigned char* texData = (unsigned char*)activeTexture->bitmap;

	do {
		const int xl = le->x;
		const int ul = le->u;
		const int ur = re->u;
		const int vl = le->v;
		const int vr = re->v;
		int length = re->x - xl;

		unsigned char *dst = vram8 + xl;
		uint32 *dst32;

		const int repDiv = dvt[length];
		const int du = ((ur - ul) * repDiv) >> DIV_TAB_SHIFT;
		const int dv = ((vr - vl) * repDiv) >> DIV_TAB_SHIFT;
		int fu = ul;
		int fv = vl;

		int xlp = xl & 3;
		if (xlp) {
			while (xlp++ < 4 && length-- > 0) {
				*dst++ = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c0 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;

			c1 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;

			c2 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;

			c3 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;

			#ifdef BIG_ENDIAN
				*dst32++ = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
			#else
				*dst32++ = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
			#endif
			length-=4;
		};

		dst = (unsigned char*)dst32;
		while (length-- > 0) {
			*dst++ = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
			fu += du;
			fv += dv;
		}

		++le;
		++re;
		vram8 += stride8;
	} while(--count > 0);
}

static void fillEnvmapEdges16(int y0, int y1)
{
	const int stride16 = softBuffer.stride >> 1;
	uint16 *vram16 = (uint16*)softBufferCurrentPtr + y0 * stride16;
	int32 *dvt = &divTab[DIV_TAB_SIZE/2];

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	const int texHeightShift = activeTexture->hShift;
	uint16* texData = (uint16*)activeTexture->bitmap;

	do {
		const int xl = le->x;
		const int ul = le->u;
		const int ur = re->u;
		const int vl = le->v;
		const int vr = re->v;
		int length = re->x - xl;

		if (length>0){
			uint16 *dst = vram16 + xl;
			uint32 *dst32;

			const int repDiv = dvt[length];
			const int du = ((ur - ul) * repDiv) >> DIV_TAB_SHIFT;
			const int dv = ((vr - vl) * repDiv) >> DIV_TAB_SHIFT;
			int fu = ul;
			int fv = vl;
			
			if (xl & 1) {
				*dst++ = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;

				length--;
			}

			dst32 = (uint32*)dst;
			while(length >= 2) {
				int c0, c1;

				c0 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;

				c1 = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;

				#ifdef BIG_ENDIAN
					*dst32++ = (c0 << 16) | c1;
				#else
					*dst32++ = (c1 << 16) | c0;
				#endif
				length -= 2;
			};

			dst = (uint16*)dst32;
			if (length != 0) {
				*dst++ = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				fu += du;
				fv += dv;
			}
		}

		++le;
		++re;
		vram16 += stride16;
	} while(--count > 0);
}

static void fillGouraudEnvmapEdges8(int y0, int y1)
{
	const int stride8 = softBuffer.stride;
	unsigned char *vram8 = (unsigned char*)softBufferCurrentPtr + y0 * stride8;
	int32 *dvt = &divTab[DIV_TAB_SIZE/2];

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	const int texHeightShift = activeTexture->hShift;
	unsigned char* texData = (unsigned char*)activeTexture->bitmap;

	do {
		const int xl = le->x;
		const int cl = le->c;
		const int cr = re->c;
		const int ul = le->u;
		const int ur = re->u;
		const int vl = le->v;
		const int vr = re->v;
		int length = re->x - xl;
		unsigned char *dst = vram8 + xl;
		uint32 *dst32;

		const int repDiv = dvt[length];
		const int dc = ((cr - cl) * repDiv) >> DIV_TAB_SHIFT;
		const int du = ((ur - ul) * repDiv) >> DIV_TAB_SHIFT;
		const int dv = ((vr - vl) * repDiv) >> DIV_TAB_SHIFT;
		int fc = cl;
		int fu = ul;
		int fv = vl;

		int c;

		int xlp = xl & 3;
		if (xlp) {
			while (xlp++ < 4 && length-- > 0) {
				c = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
				*dst++ = c + 1;

				fc += dc;
				fu += du;
				fv += dv;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c0 = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
			fc += dc;
			fu += du;
			fv += dv;

			c1 = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
			fc += dc;
			fu += du;
			fv += dv;

			c2 = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
			fc += dc;
			fu += du;
			fv += dv;

			c3 = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
			fc += dc;
			fu += du;
			fv += dv;

			#ifdef BIG_ENDIAN
				*dst32++ = ((c0 << 24) | (c1 << 16) | (c2 << 8) | c3) + 0x01010101;
			#else
				*dst32++ = ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) + 0x01010101;
			#endif
			length-=4;
		};

		dst = (unsigned char*)dst32;
		while (length-- > 0) {
			c = (texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)] * FIXED_TO_INT(fc, FP_BASE)) >> COLOR_ENVMAP_SHR;
			*dst++ = c + 1;

			fc += dc;
			fu += du;
			fv += dv;
		}

		++le;
		++re;
		vram8 += stride8;
	} while(--count > 0);
}

static void fillGouraudEnvmapEdges16(int y0, int y1)
{
	const int stride16 = softBuffer.stride >> 1;
	uint16 *vram16 = (uint16*)softBufferCurrentPtr + y0 * stride16;
	int32 *dvt = &divTab[DIV_TAB_SIZE/2];

	int count = y1 - y0 + 1;
	Edge *le = &leftEdge[y0];
	Edge *re = &rightEdge[y0];

	const int texHeightShift = activeTexture->hShift;
	uint16* texData = (uint16*)activeTexture->bitmap;

	do {
		const int xl = le->x;
		const int cl = le->c;
		const int cr = re->c;
		const int ul = le->u;
		const int ur = re->u;
		const int vl = le->v;
		const int vr = re->v;
		int r,g,b;
		int length = re->x - xl;

		if (length>0){
			uint16 *dst = vram16 + xl;
			uint32 *dst32;

			const int repDiv = dvt[length];
			const int dc = ((cr - cl) * repDiv) >> DIV_TAB_SHIFT;
			const int du = ((ur - ul) * repDiv) >> DIV_TAB_SHIFT;
			const int dv = ((vr - vl) * repDiv) >> DIV_TAB_SHIFT;
			int fc = cl;
			int fu = ul;
			int fv = vl;

			int c, cc;
			if (xl & 1) {
				c = FIXED_TO_INT(fc, FP_BASE);
				cc = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				r = (((cc >> 10) & 31) * c) >> COLOR_ENVMAP_SHR;
				g = (((cc >> 5) & 31) * c) >> COLOR_ENVMAP_SHR;
				b = ((cc  & 31) * c) >> COLOR_ENVMAP_SHR;
				*dst++ = (r << 10) | (g << 5) | b + 1;
				fc += dc;
				fu += du;
				fv += dv;

				length--;
			}

			dst32 = (uint32*)dst;
			while(length >= 2) {
				int c0, c1;

				c = FIXED_TO_INT(fc, FP_BASE);
				cc = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				r = (((cc >> 10) & 31) * c) >> COLOR_ENVMAP_SHR;
				g = (((cc >> 5) & 31) * c) >> COLOR_ENVMAP_SHR;
				b = ((cc  & 31) * c) >> COLOR_ENVMAP_SHR;
				c0 = (r << 10) | (g << 5) | b;
				fc += dc;
				fu += du;
				fv += dv;

				c = FIXED_TO_INT(fc, FP_BASE);
				cc = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				r = (((cc >> 10) & 31) * c) >> COLOR_ENVMAP_SHR;
				g = (((cc >> 5) & 31) * c) >> COLOR_ENVMAP_SHR;
				b = ((cc  & 31) * c) >> COLOR_ENVMAP_SHR;
				c1 = (r << 10) | (g << 5) | b;
				fc += dc;
				fu += du;
				fv += dv;

				#ifdef BIG_ENDIAN
					*dst32++ = (c0 << 16) | c1 + 65537;
				#else
					*dst32++ = (c1 << 16) | c0 + 65537;
				#endif

				length -= 2;
			};

			dst = (uint16*)dst32;
			if (length & 1) {
				c = FIXED_TO_INT(fc, FP_BASE);
				cc = texData[(FIXED_TO_INT(fv, FP_BASE) << texHeightShift) + FIXED_TO_INT(fu, FP_BASE)];
				r = (((cc >> 10) & 31) * c) >> COLOR_ENVMAP_SHR;
				g = (((cc >> 5) & 31) * c) >> COLOR_ENVMAP_SHR;
				b = ((cc  & 31) * c) >> COLOR_ENVMAP_SHR;
				*dst++ = (r << 10) | (g << 5) | b + 1;
				fc += dc;
				fu += du;
				fv += dv;
			}
		}

		++le;
		++re;
		vram16 += stride16;
	} while(--count > 0);
}

static bool shouldSkipTriangle(ScreenElement *e0, ScreenElement *e1, ScreenElement *e2)
{
	int outcode1 = 0, outcode2 = 0, outcode3 = 0;

	const int edgeL = 0;
	const int edgeR = SCREEN_WIDTH - 1;
	const int edgeU = 0;
	const int edgeD = SCREEN_HEIGHT - 1;

	const int x0 = e0->x + minX;
	const int y0 = e0->y + minY;
	const int x1 = e1->x + minX;
	const int y1 = e1->y + minY;
	const int x2 = e2->x + minX;
	const int y2 = e2->y + minY;

    if (y0 < edgeU) outcode1 |= 0x0001;
        else if (y0 > edgeD) outcode1 |= 0x0010;
    if (x0 < edgeL) outcode1 |= 0x0100;
        else if (x0 > edgeR) outcode1 |= 0x1000;

    if (y1 < edgeU) outcode2 |= 0x0001;
        else if (y1 > edgeD) outcode2 |= 0x0010;
    if (x1 < edgeL) outcode2 |= 0x0100;
        else if (x1 > edgeR) outcode2 |= 0x1000;

	if (y2 < edgeU) outcode3 |= 0x0001;
        else if (y2 > edgeD) outcode3 |= 0x0010;
    if (x2 < edgeL) outcode3 |= 0x0100;
        else if (x2 > edgeR) outcode3 |= 0x1000;

    return ((outcode1 & outcode2 & outcode3)!=0);
}


// 63, 32, 27, 20
// 68, 37, 26, 21


static void fillTriangleOldGouraud8(int y0, int y1, ScreenElement *e0, ScreenElement *e1, Gradients *slope1, Gradients *slope2)
{
	int count = y1 - y0;

	const int stride8 = softBuffer.stride;
	unsigned char *vram8 = (unsigned char*)softBufferCurrentPtr + y0 * stride8;
	uint32 *dst32;

	int x01 = e0->x;
	int x02 = e1->x;
	int c01 = e0->c;

	const int dx01 = slope1->dx;
	const int dx02 = slope2->dx;
	const int dc01 = slope1->dc;

	const int dc = grads.dc;

	while(count-- > 0) {
		const int sx1 = x01 >> FP_BASE;
		const int sx2 = x02 >> FP_BASE;

		int xlp = sx1 & 3;
		int sc1 = c01;
		int length = sx2 - sx1;

		unsigned char *dst = vram8 + sx1;

		if (xlp) {
			xlp = 4 - xlp;
			while (xlp-- > 0 && length-- > 0) {
				*dst++ = (unsigned char)FIXED_TO_INT(sc1, FP_BASE);
				sc1+=dc;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c0 = FIXED_TO_INT(sc1, FP_BASE);
			sc1+=dc;
			c1 = FIXED_TO_INT(sc1, FP_BASE);
			sc1+=dc;
			c2 = FIXED_TO_INT(sc1, FP_BASE);
			sc1+=dc;
			c3 = FIXED_TO_INT(sc1, FP_BASE);
			sc1+=dc;

			#ifdef BIG_ENDIAN
				*dst32++ = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
			#else
				*dst32++ = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
			#endif
			length-=4;
		}

		dst = (unsigned char*)dst32;
		while (length-- > 0) {
			*dst++ = (unsigned char)(sc1>>FP_BASE);
			sc1+=dc;
		}

		x01+=dx01;
		x02+=dx02;
		c01+=dc01;

		vram8 += stride8;
	}
}

static void fillTriangleOldGouraud16(int y0, int y1, ScreenElement *e0, ScreenElement *e1, Gradients *slope1, Gradients *slope2)
{
	const int stride16 = softBuffer.stride >> 1;
	uint16 *vram16 = (uint16*)softBufferCurrentPtr + y0 * stride16;
	uint32 *dst32;

	int count = y1 - y0;

	int x01 = e0->x;
	int x02 = e1->x;
	int c01 = e0->c;

	const int dx01 = slope1->dx;
	const int dx02 = slope2->dx;
	const int dc01 = slope1->dc;

	const int dc = grads.dc;

	while(count-- > 0) {
		const int sx1 = x01 >> FP_BASE;
		const int sx2 = x02 >> FP_BASE;

		int sc1 = c01;
		int length = sx2 - sx1;

		uint16 *dst = vram16 + sx1;

		if (length>0){
			if (sx1 & 1) {
				*dst++ = activeGradient[FIXED_TO_INT(sc1, FP_BASE)];
				sc1+=dc;
				length--;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 2) {
			int c0,c1;

			c0 = FIXED_TO_INT(sc1, FP_BASE);
			sc1+=dc;
			c1 = FIXED_TO_INT(sc1, FP_BASE);
			sc1+=dc;

			#ifdef BIG_ENDIAN
				*dst32++ = (activeGradient[c0] << 16) | activeGradient[c1];
			#else
				*dst32++ = (activeGradient[c1] << 16) | activeGradient[c0];
			#endif
			length -= 2;
		}

		dst = (uint16*)dst32;
		if (length & 1) {
			*dst++ = activeGradient[FIXED_TO_INT(sc1, FP_BASE)];
			sc1+=dc;
		}

		x01+=dx01;
		x02+=dx02;
		c01+=dc01;

		vram16 += stride16;
	}
}

static void drawTriangleOld(ScreenElement *e0, ScreenElement *e1, ScreenElement *e2)
{
	static Gradients slope01, slope12, slope02;
	static ScreenElement se0, se1, se2, se1b;

	ScreenElement *temp;
	int32 *dvt = &divTab[DIV_TAB_SIZE/2];

	if (shouldSkipTriangle(e0, e1, e2)) return;

	if (e1->y < e0->y) {
		temp = e0; e0 = e1; e1 = temp;
	}
	if (e2->y < e0->y) {
		temp = e0; e0 = e2; e2 = temp;
	}
	if (e2->y < e1->y) {
		temp = e1; e1 = e2; e2 = temp;
	}

	se0.x = e0->x << FP_BASE; se0.c = e0->c << FP_BASE;
	se1.x = e1->x << FP_BASE; se1.c = e1->c << FP_BASE;
	se2.x = e2->x << FP_BASE; se2.c = e2->c << FP_BASE;

	slope01.dx = ((se1.x - se0.x) * dvt[e1->y - e0->y]) >> DIV_TAB_SHIFT;
	slope12.dx = ((se2.x - se1.x) * dvt[e2->y - e1->y]) >> DIV_TAB_SHIFT;
	slope02.dx = ((se2.x - se0.x) * dvt[e2->y - e0->y]) >> DIV_TAB_SHIFT;
	se1b.x = se0.x + (e1->y - e0->y) * slope02.dx;

	if (renderSoftMethod & RENDER_SOFT_METHOD_GOURAUD) {
		slope01.dc = ((se1.c - se0.c) * dvt[e1->y - e0->y]) >> DIV_TAB_SHIFT;
		slope12.dc = ((se2.c - se1.c) * dvt[e2->y - e1->y]) >> DIV_TAB_SHIFT;
		slope02.dc = ((se2.c - se0.c) * dvt[e2->y - e0->y]) >> DIV_TAB_SHIFT;
		se1b.c = se0.c + (e1->y - e0->y) * slope02.dc;
	}

	if (renderSoftMethod & RENDER_SOFT_METHOD_ENVMAP) {
		slope01.du = ((se1.u - se0.u) * dvt[e1->y - e0->y]) >> DIV_TAB_SHIFT;
		slope01.dv = ((se1.v - se0.v) * dvt[e1->y - e0->y]) >> DIV_TAB_SHIFT;
		slope12.du = ((se2.u - se1.u) * dvt[e2->y - e1->y]) >> DIV_TAB_SHIFT;
		slope12.dv = ((se2.v - se1.v) * dvt[e2->y - e1->y]) >> DIV_TAB_SHIFT;
		slope02.du = ((se2.u - se0.u) * dvt[e2->y - e0->y]) >> DIV_TAB_SHIFT;
		slope02.dv = ((se2.v - se0.v) * dvt[e2->y - e0->y]) >> DIV_TAB_SHIFT;
		se1b.u = se0.u + (e1->y - e0->y) * slope02.du;
		se1b.v = se0.v + (e1->y - e0->y) * slope02.dv;
	}

	calculateTriangleGradients(e0, e1, e2);

	if (se1.x <= se1b.x) {
		fillTriangleOld(e0->y, e1->y, &se0, &se0, &slope01, &slope02);
		fillTriangleOld(e1->y, e2->y, &se1, &se1b, &slope12, &slope02);
	} else {
		fillTriangleOld(e0->y, e1->y, &se0, &se0, &slope02, &slope01);
		fillTriangleOld(e1->y, e2->y, &se1b, &se1, &slope02, &slope12);
	}
}

static void drawTriangle(ScreenElement *e0, ScreenElement *e1, ScreenElement *e2)
{
	int y0, y1;

	if (shouldSkipTriangle(e0, e1, e2)) return;

	y0 = e0->y;
	y1 = y0;

	prepareEdgeList(e0, e1);
	prepareEdgeList(e1, e2);
	prepareEdgeList(e2, e0);

	if (e1->y < y0) y0 = e1->y; if (e1->y > y1) y1 = e1->y;
	if (e2->y < y0) y0 = e2->y; if (e2->y > y1) y1 = e2->y;

	calculateTriangleGradients(e0, e1, e2);

	if (y0 < y1) {
		fillEdges(y0, y1-1);
	}
}

static void updateSoftBufferVariables(int posX, int posY, int width, int height, Mesh *ms)
{
	int celType = CEL_TYPE_UNCODED;
	int currentBufferSize;
	softBuffer.bpp = 16;
	if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
		softBuffer.bpp = 8;
		celType = CEL_TYPE_CODED;
	}

	softBuffer.width = width;
	softBuffer.height = height;
	softBuffer.stride = (((softBuffer.width * softBuffer.bpp) + 31) >> 5) << 2;	// must be multiples of 4 bytes
	if (softBuffer.stride < 8) softBuffer.stride = 8;					// and no less than 8 bytes

	currentBufferSize = (((softBuffer.stride * softBuffer.height) + 255) >> 8) << 8; // must be in multiples of 256 bytes for the unrolled optimized memset
	if (softBuffer.currentIndex + currentBufferSize > SOFT_BUFF_MAX_SIZE) {
		softBuffer.currentIndex = 0;
	}
	softBufferCurrentPtr = &softBuffer.data[softBuffer.currentIndex];
	if (currentBufferSize <= SOFT_BUFF_MAX_SIZE) {
		vramSet(0, softBufferCurrentPtr, currentBufferSize);
		//memset(softBufferCurrentPtr, 0, currentBufferSize);
		softBuffer.currentIndex += currentBufferSize;
	}	// else something went wrong

	setCelWidth(softBuffer.width, softBuffer.cel);
	setCelHeight(softBuffer.height, softBuffer.cel);
	setCelStride(softBuffer.stride, softBuffer.cel);
	setCelBpp(softBuffer.bpp, softBuffer.cel);
	setCelType(celType, softBuffer.cel);
	setCelBitmap(softBufferCurrentPtr, softBuffer.cel);
	setCelPosition(posX, posY, softBuffer.cel);
}

static void prepareAndPositionSoftBuffer(Mesh *ms, ScreenElement *elements)
{
	int i;
	const int count = ms->verticesNum;

	minX = maxX = elements[0].x;
	minY = maxY = elements[0].y;

	for (i=1; i<count; ++i) {
		const int x = elements[i].x;
		const int y = elements[i].y;
		if (x < minX) minX = x;
		if (x > maxX) maxX = x;
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
	}

	// Offset element positions to upper left min corner
	for (i=0; i<count; ++i) {
		elements[i].x -= minX;
		elements[i].y -= minY;
	}

	updateSoftBufferVariables(minX, minY, maxX - minX + 1, maxY - minY + 1, ms);
}

static bool mustUseFastGouraud(Mesh *ms)
{
	return fastGouraud && (renderSoftMethod == RENDER_SOFT_METHOD_GOURAUD) && (ms->renderType & MESH_OPTION_RENDER_SOFT8);
}

static void prepareMeshSoftRender(Mesh *ms, ScreenElement *elements)
{
	const bool useFastGouraud = mustUseFastGouraud(ms);

	if (useFastGouraud) {
		currentScanlineCel8 = scanlineCel8;
	} else {
		prepareAndPositionSoftBuffer(ms, elements);
	}

	switch(renderSoftMethod) {
		case RENDER_SOFT_METHOD_GOURAUD:
		{
			prepareEdgeList = prepareEdgeListGouraud;
			if (useFastGouraud) {
				fillEdges = fillGouraudEdges8_SemiSoft;
			} else {
				if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
					fillEdges = fillGouraudEdges8;
					fillTriangleOld = fillTriangleOldGouraud8;
				} else {
					fillEdges = fillGouraudEdges16;
					fillTriangleOld = fillTriangleOldGouraud16;
				}
			}
		}
		break;

		case RENDER_SOFT_METHOD_ENVMAP:
		{
			prepareEdgeList = prepareEdgeListEnvmap;
			if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
				fillEdges = fillEnvmapEdges8;
				fillTriangleOld = fillTriangleOldGouraud8;
			} else {
				fillEdges = fillEnvmapEdges16;
				fillTriangleOld = fillTriangleOldGouraud16;
			}
		}
		break;

		case RENDER_SOFT_METHOD_GOURAUD | RENDER_SOFT_METHOD_ENVMAP:
		{
			prepareEdgeList = prepareEdgeListGouraudEnvmap;
			if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
				fillEdges = fillGouraudEnvmapEdges8;
				fillTriangleOld = fillTriangleOldGouraud8;
			} else {
				fillEdges = fillGouraudEnvmapEdges16;
				fillTriangleOld = fillTriangleOldGouraud16;
			}
		}
		break;

		default:
		break;
	}
}

static void renderMeshSoft(Mesh *ms, ScreenElement *elements)
{
	ScreenElement *e0, *e1, *e2;
	int i,n;

	int *index = ms->index;

	prepareMeshSoftRender(ms, elements);

	for (i=0; i<ms->polysNum; ++i) {
		e0 = &elements[*index++];
		e1 = &elements[*index++];
		e2 = &elements[*index++];

		n = (e0->x - e1->x) * (e2->y - e1->y) - (e2->x - e1->x) * (e0->y - e1->y);
		if (n > 0) {
			bindMeshPolyData(ms, i);
			#ifndef OLD_TRIANGLE_DRAW
				drawTriangle(e0, e1, e2);
			#else
				drawTriangleOld(e0, e1, e2);
			#endif

			if (ms->poly[i].numPoints == 4) {	// if quad then render another triangle
				e1 = e2;
				e2 = &elements[*index];
				#ifndef OLD_TRIANGLE_DRAW
					drawTriangle(e0, e1, e2);
				#else
					drawTriangleOld(e0, e1, e2);
				#endif
			}
		}
		if (ms->poly[i].numPoints == 4) ++index;
	}

	if (mustUseFastGouraud(ms)) {
		CCB *lastScanlineCel = *(currentScanlineCel8 - 1);
		lastScanlineCel->ccb_Flags |= CCB_LAST;
		drawCels(*scanlineCel8);
		lastScanlineCel->ccb_Flags &= ~CCB_LAST;
	} else {
		drawCels(softBuffer.cel);
	}
}

static void renderMeshSoftWireframe(Mesh *ms, ScreenElement *elements)
{
	ScreenElement *e0, *e1;

	int *lineIndex = ms->lineIndex;
	int i;

	prepareMeshSoftRender(ms, elements);

	for (i=0; i<ms->linesNum; ++i) {
		e0 = &elements[*lineIndex++];
		e1 = &elements[*lineIndex++];

		bindMeshPolyData(ms, i);
		drawAntialiasedLine(e0, e1);
	}

	drawCels(softBuffer.cel);
}

void renderTransformedMeshSoft(Mesh *ms, ScreenElement *elements)
{
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

static void initSoftBuffer()
{
	softBuffer.bpp = 16;
	softBuffer.width = SCREEN_WIDTH;
	softBuffer.height = SCREEN_HEIGHT;
	softBuffer.currentIndex = 0;

	softBuffer.data = AllocMem(SOFT_BUFF_MAX_SIZE, MEMTYPE_ANY);
	softBuffer.cel = createCel(softBuffer.width, softBuffer.height, softBuffer.bpp, CEL_TYPE_UNCODED);
	setupCelData(gouraudColorShades, softBuffer.data, softBuffer.cel);
}

static void initSoftEngineArrays()
{
	leftEdge = (Edge*)AllocMem(SCREEN_HEIGHT * sizeof(Edge), MEMTYPE_ANY);
	rightEdge = (Edge*)AllocMem(SCREEN_HEIGHT * sizeof(Edge), MEMTYPE_ANY);
	divTab = (int32*)AllocMem(DIV_TAB_SIZE * sizeof(int32), MEMTYPE_ANY);

	initDivs();
}

void initEngineSoft()
{
	initSoftEngineArrays();
	if (fastGouraud) initSemiSoftGouraud();

	if (!lineColorShades[0]) lineColorShades[0] = crateColorShades(31,23,15, COLOR_GRADIENTS_SIZE, false);
	if (!lineColorShades[1]) lineColorShades[1] = crateColorShades(15,23,31, COLOR_GRADIENTS_SIZE, false);
	if (!lineColorShades[2]) lineColorShades[2] = crateColorShades(15,31,23, COLOR_GRADIENTS_SIZE, false);
	if (!lineColorShades[3]) lineColorShades[3] = crateColorShades(31,15,23, COLOR_GRADIENTS_SIZE, false);

	if (!gouraudColorShades) gouraudColorShades = crateColorShades(27,29,31, COLOR_GRADIENTS_SIZE, true);

	initSoftBuffer();
}
