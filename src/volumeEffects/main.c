#include "core.h"
#include "tools.h"

#include "effect_volumeCube.h"
#include "effect_volumeScape.h"
#include "effect_volumeScapeGradient.h"

enum { EFFECT_VOLUME_CUBE, EFFECT_VOLUME_SCAPE, EFFECT_VOLUME_SCAPE_GRADIENT, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectVolumeCubeInit, effectVolumeScapeInit, effectVolumeScapeGradientInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectVolumeCubeRun, effectVolumeScapeRun, effectVolumeScapeGradientRun };

static char *effectName[EFFECTS_NUM] = { "volume cube", "volume scape", "volume scape gradient" };

int main()
{
	//const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	const int effectIndex = EFFECT_VOLUME_SCAPE_GRADIENT;

	int extraOpts = CORE_SHOW_MEM;
	extraOpts |= (CORE_NO_VSYNC | CORE_VRAM_MAXBUFFERS);

	if (effectIndex == EFFECT_VOLUME_CUBE) extraOpts |= CORE_INIT_3D_ENGINE;


	coreInit(effectInitFunc[effectIndex], CORE_SHOW_FPS | extraOpts);
	coreRun(effectRunFunc[effectIndex]);
}
