#include "core.h"
#include "tools.h"

#include "effect_packedSprites.h"
#include "effect_packedRain.h"

enum { EFFECT_PACKED_SPRITES, EFFECT_PACKED_RAIN, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectPackedSpritesInit, effectPackedRainInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectPackedSpritesRun, effectPackedRainRun };

static char *effectName[EFFECTS_NUM] = { "packed sprites", "packed rain" };

int main()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);

	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT);
	coreRun(effectRunFunc[effectIndex]);
}
