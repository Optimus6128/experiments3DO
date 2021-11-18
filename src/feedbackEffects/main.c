#include "core.h"

#include "effect_feedbackCube.h"
#include "effect_dotCube.h"
#include "effect_mosaik.h"
#include "effect_slimecube.h"

enum { EFFECT_FEEDBACK_CUBE, EFFECT_FEEDBACK_DOTCUBE, EFFECT_FEEDBACK_MOSAIK, EFFECT_FEEDBACK_SLIMECUBE, EFFECTS_NUM };

static void(*effectInitFunc[EFFECTS_NUM])() = { effectFeedbackCubeInit, effectDotcubeInit, effectMosaikInit, effectSlimecubeInit };
static void(*effectRunFunc[EFFECTS_NUM])() = { effectFeedbackCubeRun, effectDotcubeRun, effectMosaikRun, effectSlimecubeRun };

static int effectIndex = EFFECT_FEEDBACK_CUBE;

int main()
{
	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT | CORE_VRAM_BUFFERS(2) | CORE_OFFSCREEN_BUFFERS(4));
	coreRun(effectRunFunc[effectIndex]);
}
