#include "core.h"
#include "tools.h"

#include "effect_meshPyramids.h"
#include "effect_meshGrid.h"
#include "effect_meshSoft.h"
#include "effect_meshWorld.h"
#include "effect_meshLoad.h"
#include "effect_meshParticles.h"

enum { EFFECT_MESH_PYRAMIDS, EFFECT_MESH_GRID, EFFECT_MESH_SOFT, EFFECT_MESH_WORLD, EFFECT_MESH_LOAD, EFFECT_MESH_PARTICLES, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectMeshPyramidsInit, effectMeshGridInit, effectMeshSoftInit, effectMeshWorldInit, effectMeshLoadInit, effectMeshParticlesInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectMeshPyramidsRun, effectMeshGridRun, effectMeshSoftRun, effectMeshWorldRun, effectMeshLoadRun, effectMeshParticlesRun };

static char *effectName[EFFECTS_NUM] = { "mesh pyramids test", "mesh grid", "software 3d", "3d world", "mesh load", "particles" };

int main()
{
	int extraOpts = CORE_SHOW_MEM;

	//const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	const int effectIndex = EFFECT_MESH_PARTICLES;

	if (effectIndex == EFFECT_MESH_SOFT || effectIndex == EFFECT_MESH_WORLD) extraOpts |= CORE_INIT_3D_ENGINE_SOFT;
	if (effectIndex != EFFECT_MESH_PYRAMIDS) extraOpts |= CORE_NO_VSYNC;

	coreInit(effectInitFunc[effectIndex], CORE_SHOW_FPS | CORE_INIT_3D_ENGINE | CORE_VRAM_MAXBUFFERS | extraOpts);
	coreRun(effectRunFunc[effectIndex]);
}
