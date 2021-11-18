#include "core.h"
#include "tools.h"

#include "effect_feedbackCube.h"
#include "effect_dotCube.h"
#include "effect_mosaik.h"
#include "effect_slimecube.h"

enum { EFFECT_FEEDBACK_CUBE, EFFECT_FEEDBACK_DOTCUBE, EFFECT_FEEDBACK_MOSAIK, EFFECT_FEEDBACK_SLIMECUBE, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectFeedbackCubeInit, effectDotcubeInit, effectMosaikInit, effectSlimecubeInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectFeedbackCubeRun, effectDotcubeRun, effectMosaikRun, effectSlimecubeRun };

static char *effectName[EFFECTS_NUM] = { "feedback cube", "dot cube", "mosaik effect", "slime cube" };

int main()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);

	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT | CORE_VRAM_BUFFERS(2) | CORE_OFFSCREEN_BUFFERS(4));
	coreRun(effectRunFunc[effectIndex]);
}
