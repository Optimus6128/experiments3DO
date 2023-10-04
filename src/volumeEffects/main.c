#include "core.h"
#include "tools.h"

#include "main.h"

int main()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);

	int extraOpts = CORE_SHOW_MEM;
	extraOpts |= (CORE_NO_VSYNC | CORE_VRAM_MAXBUFFERS);

	if (effectIndex == EFFECT_VOLUME_CUBE) extraOpts |= CORE_INIT_3D_ENGINE;


	coreInit(effectInitFunc[effectIndex], CORE_SHOW_FPS | extraOpts);
	coreRun(effectRunFunc[effectIndex]);
}
