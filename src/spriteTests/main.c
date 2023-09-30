#include "core.h"
#include "tools.h"

#include "effect_spritesGecko.h"
#include "effect_layers.h"
#include "effect_parallax.h"
#include "effect_julia.h"
#include "effect_water.h"
#include "effect_sphere.h"
#include "effect_fliAnimTest.h"
#include "effect_amv.h"

enum { EFFECT_SPRITES_GECKO, EFFECT_LAYERS, EFFECT_PARALLAX, EFFECT_JULIA, EFFECT_WATER, EFFECT_SPHERE, EFFECT_FLI_ANIM_TEST, EFFECT_AMV, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectSpritesGeckoInit, effectLayersInit, effectParallaxInit, effectJuliaInit, effectWaterInit, effectSphereInit, effectFliAnimTestInit, effectAmvInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectSpritesGeckoRun, effectLayersRun, effectParallaxRun, effectJuliaRun, effectWaterRun, effectSphereRun, effectFliAnimTestRun, effectAmvRun };

//static char *effectName[EFFECTS_NUM] = { "1920 gecko sprites", "background layers", "parallax tests", "julia fractal", "water ripples", "sphere mapping", "fli animation test", "AMV bits" };

int main()
{
	const int effectIndex = EFFECT_AMV; //runEffectSelector(effectName, EFFECTS_NUM);
	int coreFlags = CORE_DEFAULT;

	if (effectIndex == EFFECT_FLI_ANIM_TEST) {
		coreFlags = CORE_SHOW_FPS | CORE_NO_VSYNC | CORE_NO_CLEAR_FRAME;
	}

	coreInit(effectInitFunc[effectIndex], coreFlags);
	coreRun(effectRunFunc[effectIndex]);
}
