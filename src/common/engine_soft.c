#include "core.h"

#include "tools.h"

#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"

#include "sprite_engine.h"

#include "system_graphics.h"

#include "mathutil.h"

static Sprite *sprSoftBuffer;

static ubyte softBuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

static void renderSoftQuadOnScreen()
{
	drawSprite(sprSoftBuffer);
}

static void renderMeshSoft(Mesh *ms, Vertex *vertices)
{
	Point quad[4];
	int *indices = ms->index;

	int i,n;
	for (i=0; i<ms->indexNum; i+=4)
	{
		quad[0].pt_X = vertices[indices[i]].x; quad[0].pt_Y = vertices[indices[i]].y;
		quad[1].pt_X = vertices[indices[i+1]].x; quad[1].pt_Y = vertices[indices[i+1]].y;
		quad[2].pt_X = vertices[indices[i+2]].x; quad[2].pt_Y = vertices[indices[i+2]].y;
		quad[3].pt_X = vertices[indices[i+3]].x; quad[3].pt_Y = vertices[indices[i+3]].y;

		n = (quad[0].pt_X - quad[1].pt_X) * (quad[2].pt_Y - quad[1].pt_Y) - (quad[2].pt_X - quad[1].pt_X) * (quad[0].pt_Y - quad[1].pt_Y);

		if (n > 0) {
			// render
		}
	}
}

void renderTransformedMeshSoft(Mesh *ms, Vertex *vertices)
{
	renderMeshSoft(ms, vertices);

	renderSoftQuadOnScreen();
}

static void testInitRandomTexture()
{
	const int size = sprSoftBuffer->width * sprSoftBuffer->height;
	ubyte *sprData = (ubyte*)getSpriteBitmapData(sprSoftBuffer);
	int i;

	for (i=0; i<size; ++i) {
		*sprData++ = rand() & 31;
	}
}

void initEngineSoft()
{
	static uint16 softBufferPal[32];

	setPal(0,31, 0,0,0, 255,255,255, softBufferPal, 3);

	sprSoftBuffer = newSprite(SCREEN_WIDTH, SCREEN_HEIGHT, 8, CREATECEL_CODED, softBufferPal, softBuffer);

	testInitRandomTexture();
}
