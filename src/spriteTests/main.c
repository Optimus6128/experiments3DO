#include "core.h"
#include "tools.h"

#include "effect_spritesGecko.h"
#include "effect_layers.h"
#include "effect_parallax.h"

enum { EFFECT_SPRITES_GECKO, EFFECT_LAYERS, EFFECT_PARALLAX, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectSpritesGeckoInit, effectLayersInit, effectParallaxInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectSpritesGeckoRun, effectLayersRun, effectParallaxRun };

static char *effectName[EFFECTS_NUM] = { "1920 gecko sprites", "background layers", "parallax tests" };

int main()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);

	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT);
	coreRun(effectRunFunc[effectIndex]);
}
