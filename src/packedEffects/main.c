#include "core.h"
#include "tools.h"

#include "effect_packedSprites.h"
#include "effect_packedRain.h"
#include "effect_packedRadial.h"

enum { EFFECT_PACKED_SPRITES, EFFECT_PACKED_RAIN, EFFECT_PACKED_RADIAL, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectPackedSpritesInit, effectPackedRainInit, effectPackedRadialInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectPackedSpritesRun, effectPackedRainRun, effectPackedRadialRun };

static char *effectName[EFFECTS_NUM] = { "packed sprites", "packed rain", "packed radial" };

int main()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);

	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT | CORE_SHOW_MEM);
	coreRun(effectRunFunc[effectIndex]);
}
