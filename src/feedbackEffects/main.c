#include "core.h"
#include "tools.h"

#include "main.h"

int main()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);

	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT | CORE_VRAM_BUFFERS(2) | CORE_OFFSCREEN_BUFFERS(4) | CORE_INIT_3D_ENGINE);
	coreRun(effectRunFunc[effectIndex]);
}
