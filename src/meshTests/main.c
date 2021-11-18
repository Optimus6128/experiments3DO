#include "core.h"

#include "effect_meshPyramids.h"
#include "effect_meshSoft.h"

enum { EFFECT_MESH_PYRAMIDS, EFFECT_MESH_SOFT, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectMeshPyramidsInit, effectMeshSoftInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectMeshPyramidsRun, effectMeshSoftRun };

static int effectIndex = EFFECT_MESH_PYRAMIDS;

int main()
{
	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT);
	coreRun(effectRunFunc[effectIndex]);
}
