#include "core.h"

#include "tools.h"

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


typedef struct Edge
{
	int x;
	int c;
}Edge;

typedef struct VrtxElement
{
	int x,y;
	int c;
}VrtxElement;

static Sprite *sprSoftBuffer = NULL;
static Sprite *sprSoftBuffer8 = NULL;
static Sprite *sprSoftBuffer16 = NULL;

static uint8 softBuffer8[SOFT_BUFF_WIDTH * SOFT_BUFF_HEIGHT];
static uint16 softBuffer16[SOFT_BUFF_WIDTH * SOFT_BUFF_HEIGHT];

static Edge leftEdge[SOFT_BUFF_HEIGHT];
static Edge rightEdge[SOFT_BUFF_HEIGHT];

static int32 divTab[DIV_TAB_SIZE];

static uint16 *lineColorShades[4] = { NULL, NULL, NULL, NULL };

static void(*fillGouraudEdges)(int yMin, int yMax, uint16 *colorShades);

#define LN_BASE 8
#define LN_AND ((1 << LN_BASE) - 1)

static uint16 *crateColorShades(int r, int g, int b, int numShades) {
	uint16 *colorShades = (uint16*)AllocMem(sizeof(uint16) * numShades, MEMTYPE_ANY);

	setPalGradient(0, numShades-1, 0,1,2, r,g,b, colorShades);

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


static void prepareEdgeListGouraud(VrtxElement *ve0, VrtxElement *ve1)
{
	Edge *edgeListToWriteFlat;
	VrtxElement *veTemp;

	if (ve0->y == ve1->y) return;

	// Assumes CCW
	if (ve0->y < ve1->y) {
		edgeListToWriteFlat = leftEdge;
	}
	else {
		edgeListToWriteFlat = rightEdge;

		veTemp = ve0;
		ve0 = ve1;
		ve1 = veTemp;
	}

    {
        const int x0 = ve0->x; int y0 = ve0->y; int c0 = ve0->c;
        const int x1 = ve1->x; int y1 = ve1->y; int c1 = ve1->c;

        int dy = y1 - y0;
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

        edgeListToWriteFlat = &edgeListToWriteFlat[y0];
        do {
			int x = FIXED_TO_INT(fx, FP_BASE);
			//int c = FIXED_TO_INT(fc, FP_BASE);
			CLAMP(x, 0, SOFT_BUFF_WIDTH-1)
			//CLAMP(c, 0, COLOR_GRADIENTS_SIZE-1)
			edgeListToWriteFlat->x = x;
			edgeListToWriteFlat->c = fc;
            ++edgeListToWriteFlat;
            fx += dx;
			fc += dc;
		} while(--dy > 0);
    }
}

static void fillGouraudEdges8(int yMin, int yMax, uint16 *colorShades)
{
	uint8 *vram8 = softBuffer8 + yMin * SOFT_BUFF_WIDTH;
	int count = yMax - yMin;
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
				CLAMP(c, 0, COLOR_GRADIENTS_SIZE-1)
				fc += dc;
				*dst++ = c;
			}
		}

		dst32 = (uint32*)dst;
		while(length >= 4) {
			int c0,c1,c2,c3;

			c0 = FIXED_TO_INT(fc, FP_BASE);
			//CLAMP(c0, 0, COLOR_GRADIENTS_SIZE-1)
			fc += dc;
			c1 = FIXED_TO_INT(fc, FP_BASE);
			//CLAMP(c1, 0, COLOR_GRADIENTS_SIZE-1)
			fc += dc;
			c2 = FIXED_TO_INT(fc, FP_BASE);
			//CLAMP(c2, 0, COLOR_GRADIENTS_SIZE-1)
			fc += dc;
			c3 = FIXED_TO_INT(fc, FP_BASE);
			//CLAMP(c3, 0, COLOR_GRADIENTS_SIZE-1)
			fc += dc;
			*dst32++ = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
			length-=4;
		};

		dst = (uint8*)dst32;
		while (length-- > 0) {
			int c = FIXED_TO_INT(fc, FP_BASE);
			CLAMP(c, 0, COLOR_GRADIENTS_SIZE-1)
			fc += dc;
			*dst++ = c;
		}

		++le;
		++re;
		vram8 += SOFT_BUFF_WIDTH;
	} while(--count > 0);
}

/*static void fillGouraudEdges8(int yMin, int yMax, uint16 *colorShades)
{
	uint8 *vram8 = softBuffer8 + yMin * SOFT_BUFF_WIDTH;
	int count = yMax - yMin;
	Edge *le = &leftEdge[yMin];
	Edge *re = &rightEdge[yMin];
	do {
		const int xl = le->x;
		const int cl = le->c;
		const int cr = re->c;
		int length = re->x - xl;
		uint8 *dst = vram8 + xl;

		const int repDiv = divTab[length + DIV_TAB_SIZE / 2];
		const int dc = (((cr - cl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		int fc = cl;

		while(length-- >= 0) {
			int c = FIXED_TO_INT(fc, FP_BASE);
			CLAMP(c, 0, COLOR_GRADIENTS_SIZE-1)
			*dst++ = c;
			fc += dc;
		};

		++le;
		++re;
		vram8 += SOFT_BUFF_WIDTH;
	} while(--count > 0);
}*/

static void fillGouraudEdges16(int yMin, int yMax, uint16 *colorShades)
{
	uint16 *vram16 = softBuffer16 + yMin * SOFT_BUFF_WIDTH;
	int count = yMax - yMin;
	Edge *le = &leftEdge[yMin];
	Edge *re = &rightEdge[yMin];
	do {
		const int xl = le->x;
		const int cl = le->c;
		const int cr = re->c;
		int length = re->x - xl;
		uint16 *dst = vram16 + xl;

		const int repDiv = divTab[length + DIV_TAB_SIZE / 2];
		const int dc = (((cr - cl) * repDiv) >>  (DIV_TAB_SHIFT - FP_BASE)) >> FP_BASE;
		int fc = cl;

		while(length-- >= 0) {
			int c = FIXED_TO_INT(fc, FP_BASE);
			CLAMP(c, 0, COLOR_GRADIENTS_SIZE-1)
			*dst++ = colorShades[c];
			fc += dc;
		};

		++le;
		++re;
		vram16 += SOFT_BUFF_WIDTH;
	} while(--count > 0);
}

static void drawGouraudTriangle(VrtxElement *ves, uint16 *colorShades)
{
	int yMin = ves[0].y;
	int yMax = yMin;

	prepareEdgeListGouraud(&ves[0], &ves[1]);
	prepareEdgeListGouraud(&ves[1], &ves[2]);
	prepareEdgeListGouraud(&ves[2], &ves[0]);

	{
		const int y1 = ves[1].y;
		const int y2 = ves[2].y;
		if (y1 < yMin) yMin = y1; if (y1 > yMax) yMax = y1;
		if (y2 < yMin) yMin = y2; if (y2 > yMax) yMax = y2;
	}

	if (yMin < 0) yMin = 0;
	if (yMax > SOFT_BUFF_HEIGHT-1) yMax = SOFT_BUFF_HEIGHT-1;

	fillGouraudEdges(yMin, yMax, colorShades);
}

/*static void drawAntialiasedLine(Vertex *p1, Vertex *p2, uint16 *colorShades16)
{
	int x1 = p1->x;
	int y1 = p1->y;
	int x2 = p2->x;
	int y2 = p2->y;

	int dx, dy, l;
	int x00, y00;
	int vramofs;

	int x, y;
	int frac, shade;

	int temp;
    int chdx, chdy;

	uint16 *vram = (uint16*)softBuffer;

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
			temp = y1; y1 = y2; y2 = temp;
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
				*(vram + vramofs) |= colorShades16[shade];

				shade = frac >> 4;
				*(vram + vramofs+SOFT_BUFF_WIDTH) |= colorShades16[shade];
			}
            y00+=l;
		}
	}
	else {
		if (y1 > y2) {
			temp = x1; x1 = x2; x2 = temp;
			temp = y1; y1 = y2; y2 = temp;
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
				*(vram + vramofs) |= colorShades16[shade];

				shade = frac >> 4;
				*(vram + vramofs + 1) |= colorShades16[shade];
			}
            x00+=l;
		}
	}
}

static void renderPlots(Vertex *pt)
{
	if (pt->x < 0 || pt->x > SOFT_BUFF_WIDTH-1 || pt->y < 0 || pt->y > SOFT_BUFF_HEIGHT-1 || pt->z <= 0) return;

	softBuffer[pt->y * SOFT_BUFF_WIDTH + pt->x] = 0xFFFF;
}

static void renderMeshSoftWireframe(Mesh *ms, Vertex *vertices)
{
	Vertex *pt0, *pt1;

	int *lineIndex = ms->lineIndex;
	int i;
	for (i=0; i<ms->linesNum; ++i) {
		pt0 = &vertices[*lineIndex++];
		pt1 = &vertices[*lineIndex++];

		drawAntialiasedLine(pt0, pt1, lineColorShades[i & 3]);
	}
}*/

static void renderMeshSoft(Mesh *ms, Vertex *vertices)
{
	static VrtxElement vrtxElements[3];
	Vertex *pt0, *pt1, *pt2;
	int c0, c1, c2;
	int i,n;

	int *index = ms->index;
	int *vertexCol = ms->vertexCol;
	
	fillGouraudEdges = fillGouraudEdges16;
	if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
		fillGouraudEdges = fillGouraudEdges8;
	}

	for (i=0; i<ms->polysNum; ++i) {
		pt0 = &vertices[*index]; c0 = vertexCol[*index++];
		pt1 = &vertices[*index]; c1 = vertexCol[*index++];
		pt2 = &vertices[*index]; c2 = vertexCol[*index++];

		n = (pt0->x - pt1->x) * (pt2->y - pt1->y) - (pt2->x - pt1->x) * (pt0->y - pt1->y);
		if (n > 0) {
			vrtxElements[0].x = pt0->x; vrtxElements[0].y = pt0->y; vrtxElements[0].c = c0;
			vrtxElements[1].x = pt1->x; vrtxElements[1].y = pt1->y; vrtxElements[1].c = c1;
			vrtxElements[2].x = pt2->x; vrtxElements[2].y = pt2->y; vrtxElements[2].c = c2;

			drawGouraudTriangle(vrtxElements, lineColorShades[i & 3]);
		}
	}
}

static void clearSoftBuffer()
{
	vramSet(0x01010101, (void*)getSpriteBitmapData(sprSoftBuffer), getCelDataSizeInBytes(sprSoftBuffer->cel));
}

static void renderSoftQuadOnScreen()
{
	drawSprite(sprSoftBuffer);
}

void renderTransformedMeshSoft(Mesh *ms, Vertex *vertices)
{
	sprSoftBuffer = sprSoftBuffer16;
	if (ms->renderType & MESH_OPTION_RENDER_SOFT8) {
		sprSoftBuffer = sprSoftBuffer8;
	}

	clearSoftBuffer();

	renderMeshSoft(ms, vertices);
	//renderMeshSoftWireframe(ms, vertices);

	renderSoftQuadOnScreen();
}


void initEngineSoft()
{
	if (!sprSoftBuffer8) sprSoftBuffer8 = newSprite(SOFT_BUFF_WIDTH, SOFT_BUFF_HEIGHT, 8, CREATECEL_CODED, NULL, (void*)softBuffer8);
	if (!sprSoftBuffer16) sprSoftBuffer16 = newSprite(SOFT_BUFF_WIDTH, SOFT_BUFF_HEIGHT, 16, CREATECEL_UNCODED, NULL, (void*)softBuffer16);

	initDivs();

	if (!lineColorShades[0]) lineColorShades[0] = crateColorShades(31,23,15, COLOR_GRADIENTS_SIZE);
	if (!lineColorShades[1]) lineColorShades[1] = crateColorShades(15,23,31, COLOR_GRADIENTS_SIZE);
	if (!lineColorShades[2]) lineColorShades[2] = crateColorShades(15,31,23, COLOR_GRADIENTS_SIZE);
	if (!lineColorShades[3]) lineColorShades[3] = crateColorShades(31,15,23, COLOR_GRADIENTS_SIZE);

	sprSoftBuffer8->cel->ccb_PLUTPtr = (PLUTChunk*)lineColorShades[1];
}
