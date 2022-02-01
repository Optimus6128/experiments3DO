#include "core.h"
#include "tools.h"

#include "effect_spritesGecko.h"

//enum { EFFECT_SPRITES_GECKO, EFFECTS_NUM };

//static void(*effectInitFunc[EFFECTS_NUM])() = { effectSpritesGeckoInit };
//static void(*effectRunFunc[EFFECTS_NUM])() = { effectSpritesGeckoRun };

//static char *effectName[EFFECTS_NUM] = { "1920 gecko sprites" };

int main()
{
	//const int effectIndex = EFFECT_SPRITES_GECKO;//runEffectSelector(effectName, EFFECTS_NUM);

//	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT);
//	coreRun(effectRunFunc[effectIndex]);

	coreInit(effectSpritesGeckoInit, CORE_DEFAULT);
	coreRun(effectSpritesGeckoRun);
}
