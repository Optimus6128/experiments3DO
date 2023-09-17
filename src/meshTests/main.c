#include "core.h"
#include "tools.h"

#include "effect_meshPyramids.h"
#include "effect_meshGrid.h"
#include "effect_meshSoft.h"
#include "effect_meshWorld.h"
#include "effect_meshSkybox.h"
#include "effect_meshLoad.h"
#include "effect_meshParticles.h"
#include "effect_meshHeightmap.h"
#include "effect_meshFli.h"

enum { EFFECT_MESH_PYRAMIDS, EFFECT_MESH_GRID, EFFECT_MESH_SOFT, EFFECT_MESH_WORLD, EFFECT_MESH_SKYBOX, EFFECT_MESH_LOAD, EFFECT_MESH_PARTICLES, EFFECT_MESH_HEIGHTMAP, EFFECT_MESH_FLI, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectMeshPyramidsInit, effectMeshGridInit, effectMeshSoftInit, effectMeshWorldInit, effectMeshSkyboxInit, effectMeshLoadInit, effectMeshParticlesInit, effectMeshHeightmapInit, effectMeshFliInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectMeshPyramidsRun, effectMeshGridRun, effectMeshSoftRun, effectMeshWorldRun, effectMeshSkyboxRun, effectMeshLoadRun, effectMeshParticlesRun, effectMeshHeightmapRun, effectMeshFliRun };

//static char *effectName[EFFECTS_NUM] = { "mesh pyramids test", "mesh grid", "software 3d", "3d world", "skybox", "mesh load", "particles", "heightmap", "FLI plane" };

int main()
{
	int extraOpts = CORE_SHOW_MEM;

	//const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	const int effectIndex = EFFECT_MESH_SKYBOX;

	if (effectIndex == EFFECT_MESH_SOFT || effectIndex == EFFECT_MESH_WORLD) extraOpts |= CORE_INIT_3D_ENGINE_SOFT;
	//if (effectIndex != EFFECT_MESH_PYRAMIDS) extraOpts |= (CORE_NO_VSYNC | CORE_VRAM_MAXBUFFERS);
	//if (effectIndex != EFFECT_MESH_FLI) extraOpts &= ~CORE_VRAM_MAXBUFFERS;

	coreInit(effectInitFunc[effectIndex], CORE_SHOW_FPS | CORE_INIT_3D_ENGINE | extraOpts);
	coreRun(effectRunFunc[effectIndex]);
}
