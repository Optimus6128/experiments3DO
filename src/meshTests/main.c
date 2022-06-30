#include "core.h"
#include "tools.h"

#include "effect_meshPyramids.h"
#include "effect_meshGrid.h"
#include "effect_meshSoft.h"

enum { EFFECT_MESH_PYRAMIDS, EFFECT_MESH_GRID, EFFECT_MESH_SOFT, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectMeshPyramidsInit, effectMeshGridInit, effectMeshSoftInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectMeshPyramidsRun, effectMeshGridRun, effectMeshSoftRun };

static char *effectName[EFFECTS_NUM] = { "mesh pyramids test", "mesh grid", "software 3d" };

int main()
{
	//const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	const int effectIndex = EFFECT_MESH_GRID;

	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT_INPUT | CORE_SHOW_FPS | CORE_VRAM_MAXBUFFERS | CORE_INIT_3D_ENGINE_SOFT);	// max buffersto not flicker when vsync off and clear screen on
	coreRun(effectRunFunc[effectIndex]);
}
