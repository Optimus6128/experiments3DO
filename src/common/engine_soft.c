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

	int *index = ms->index;	// will assume all triangles for the soft renderer for now
	for (i=0; i<ms->polysNum; ++i) {
		pt0 = &vertices[*index++];
		pt1 = &vertices[*index++];
		pt2 = &vertices[*index++];

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
