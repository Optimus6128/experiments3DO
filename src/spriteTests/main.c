#include "core.h"
#include "tools.h"

#include "main.h"

int main()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	int coreFlags = CORE_DEFAULT;

	if (effectIndex == EFFECT_FLI_ANIM_TEST) {
		coreFlags = CORE_SHOW_FPS | CORE_NO_VSYNC | CORE_NO_CLEAR_FRAME;
	}

	if (effectIndex == EFFECT_RAYTRACE) {
		coreFlags = CORE_SHOW_FPS | CORE_SHOW_MEM | CORE_INIT_3D_ENGINE;
	}

	coreInit(effectInitFunc[effectIndex], coreFlags);
	coreRun(effectRunFunc[effectIndex]);
}
