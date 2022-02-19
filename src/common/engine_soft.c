#include "core.h"

#include "tools.h"

#include "engine_mesh.h"
#include "engine_texture.h"
#include "engine_soft.h"

#include "sprite_engine.h"

#include "system_graphics.h"

#include "mathutil.h"

static Sprite *sprSoftBuffer = NULL;

static ubyte softBuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 2];

static void renderSoftQuadOnScreen()
{
	drawSprite(sprSoftBuffer);
}

static void renderMeshSoft(Mesh *ms, Vertex *vertices)
{
	Vertex *pt0, *pt1, *pt2;
	int i,n;

	int *currIndex = &ms->index[ms->quadsNum];	// at the end of quad indices start the triangle indices
	for (i=0; i<ms->trianglesNum; ++i) {
		pt0 = &vertices[*currIndex++];
		pt1 = &vertices[*currIndex++];
		pt2 = &vertices[*currIndex++];

		n = (pt0->x - pt1->x) * (pt2->y - pt1->y) - (pt2->x - pt1->x) * (pt0->y - pt1->y);
		if (n > 0) {

		}
	}
}

static void clearSoftBuffer()
{
	memset(getSpriteBitmapData(sprSoftBuffer), 0, sprSoftBuffer->width * sprSoftBuffer->height * 2);
}

void renderTransformedMeshSoft(Mesh *ms, Vertex *vertices)
{
	clearSoftBuffer();

	renderMeshSoft(ms, vertices);

	renderSoftQuadOnScreen();
}



void initEngineSoft()
{
	if (!sprSoftBuffer) sprSoftBuffer = newSprite(SCREEN_WIDTH, SCREEN_HEIGHT, 16, CREATECEL_UNCODED, NULL, softBuffer);
}
