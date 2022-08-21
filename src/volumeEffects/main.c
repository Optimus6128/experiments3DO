#include "core.h"
#include "tools.h"

#include "effect_volumeCube.h"
#include "effect_volumeScape.h"

enum { EFFECT_VOLUME_CUBE, EFFECT_VOLUME_HEIGHTMAP, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectVolumeCubeInit, effectVolumeScapeInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectVolumeCubeRun, effectVolumeScapeRun };

static char *effectName[EFFECTS_NUM] = { "volume cube", "volume scape" };

int main()
{
	//const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);
	const int effectIndex = EFFECT_VOLUME_HEIGHTMAP;

	coreInit(effectInitFunc[effectIndex], CORE_SHOW_FPS | CORE_NO_VSYNC | CORE_INIT_3D_ENGINE | CORE_VRAM_MAXBUFFERS);
	coreRun(effectRunFunc[effectIndex]);
}
