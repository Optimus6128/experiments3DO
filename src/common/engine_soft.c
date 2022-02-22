#include "core.h"

#include "tools.h"

#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"

#include "sprite_engine.h"

#include "system_graphics.h"

#include "mathutil.h"
#include "tools.h"

static Sprite *sprSoftBuffer = NULL;

static uint16 softBuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

static uint16 *lineColorShades[4] = { NULL, NULL, NULL, NULL };


#define LN_BASE 8
#define LN_AND ((1 << LN_BASE) - 1)

uint16 *crateColorShades(int r, int g, int b, int numShades) {
	uint16 *colorShades = (uint16*)AllocMem(sizeof(uint16) * numShades, MEMTYPE_ANY);

	setPalGradient(0, numShades-1, 0,0,0, r,g,b, colorShades);

	return colorShades;
}

static void drawAntialiasedLine(Vertex *p1, Vertex *p2, uint16 *colorShades16)
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
        else if (y1 > SCREEN_HEIGHT-2) outcode1 |= 0x0010;
    if (x1 < 1) outcode1 |= 0x0100;
        else if (x1 > SCREEN_WIDTH-2) outcode1 |= 0x1000;

    if (y2 < 1) outcode2 |= 0x0001;
        else if (y2 > SCREEN_HEIGHT-2) outcode2 |= 0x0010;
    if (x2 < 1) outcode2 |= 0x0100;
        else if (x2 > SCREEN_WIDTH-2) outcode2 |= 0x1000;

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

			if (x >= 0 && x < SCREEN_WIDTH && yp >=0 && yp < SCREEN_HEIGHT - 1) {
				vramofs = yp*SCREEN_WIDTH + x;
				frac = y00 & LN_AND;

				shade = (LN_AND - frac) >> 4;
				*(vram + vramofs) |= colorShades16[shade];

				shade = frac >> 4;
				*(vram + vramofs+SCREEN_WIDTH) |= colorShades16[shade];
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

			if (y >= 0 && y < SCREEN_HEIGHT && xp >=0 && xp < SCREEN_WIDTH - 1) {
				vramofs = y*SCREEN_WIDTH + xp;
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
	if (pt->x < 0 || pt->x > SCREEN_WIDTH-1 || pt->y < 0 || pt->y > SCREEN_HEIGHT-1 || pt->z <= 0) return;

	softBuffer[pt->y * SCREEN_WIDTH + pt->x] = 0xFFFF;
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
}

static void renderMeshSoft(Mesh *ms, Vertex *vertices)
{
	Vertex *pt0, *pt1, *pt2;
	int i,n;

	int *index = ms->index;	// will assume all triangles for the soft renderer for now

	for (i=0; i<ms->polysNum; ++i) {
		pt0 = &vertices[*index++];
		pt1 = &vertices[*index++];
		pt2 = &vertices[*index++];

		n = (pt0->x - pt1->x) * (pt2->y - pt1->y) - (pt2->x - pt1->x) * (pt0->y - pt1->y);
		if (n > 0) {
			
		}

		renderPlots(pt0);
		renderPlots(pt1);
		renderPlots(pt2);
	}
}

static void clearSoftBuffer()
{
	//memset(getSpriteBitmapData(sprSoftBuffer), 0, sprSoftBuffer->width * sprSoftBuffer->height * 2);
	vramSet(0, (void*)getSpriteBitmapData(sprSoftBuffer));
}

static void renderSoftQuadOnScreen()
{
	drawSprite(sprSoftBuffer);
}

void renderTransformedMeshSoft(Mesh *ms, Vertex *vertices)
{
	clearSoftBuffer();

	renderMeshSoft(ms, vertices);
	//renderMeshSoftWireframe(ms, vertices);

	renderSoftQuadOnScreen();
}



void initEngineSoft()
{
	if (!sprSoftBuffer) sprSoftBuffer = newSprite(SCREEN_WIDTH, SCREEN_HEIGHT, 16, CREATECEL_UNCODED, NULL, (void*)softBuffer);

	if (!lineColorShades[0]) lineColorShades[0] = crateColorShades(31,23,15, 16);
	if (!lineColorShades[1]) lineColorShades[1] = crateColorShades(15,23,31, 16);
	if (!lineColorShades[2]) lineColorShades[2] = crateColorShades(15,31,23, 16);
	if (!lineColorShades[3]) lineColorShades[3] = crateColorShades(31,15,23, 16);

}
