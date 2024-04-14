#include "core.h"
#include "tools.h"

#include "main.h"

int main()
{
	int extraOpts = (CORE_NO_VSYNC | CORE_VRAM_MAXBUFFERS);//CORE_SHOW_MEM;

	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	//const int effectIndex = EFFECT_MESH_SOFT;

	if (effectIndex == EFFECT_MESH_SOFT || effectIndex == EFFECT_MESH_WORLD) extraOpts |= CORE_INIT_3D_ENGINE_SOFT;
	//if (effectIndex != EFFECT_MESH_PYRAMIDS) extraOpts |= (CORE_NO_VSYNC | CORE_VRAM_MAXBUFFERS);
	//if (effectIndex != EFFECT_MESH_FLI) extraOpts &= ~CORE_VRAM_MAXBUFFERS;

	coreInit(effectInitFunc[effectIndex], CORE_SHOW_FPS | CORE_INIT_3D_ENGINE | extraOpts);
	ScavengeMem();
	coreRun(effectRunFunc[effectIndex]);
}
