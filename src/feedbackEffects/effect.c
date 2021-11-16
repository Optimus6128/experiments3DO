#include "effect.h"
#include "effect_feedbackCube.h"
#include "effect_dotCube.h"
#include "effect_mosaik.h"
#include "effect_slimecube.h"

enum { FEEDBACK_EFFECT_CUBE, FEEDBACK_EFFECT_DOTCUBE, FEEDBACK_EFFECT_MOSAIK, FEEDBACK_EFFECT_SLIMECUBE };

static int feedbackEffect = FEEDBACK_EFFECT_MOSAIK;

void effectInit()
{
	switch (feedbackEffect) {
		case FEEDBACK_EFFECT_CUBE:
			effectFeedbackCubeInit();
		break;
		
		case FEEDBACK_EFFECT_DOTCUBE:
			effectDotcubeInit();
		break;

		case FEEDBACK_EFFECT_MOSAIK:
			effectMosaikInit();
		break;

		case FEEDBACK_EFFECT_SLIMECUBE:
			effectSlimecubeInit();
		break;
	}
}

void effectRun()
{
	switch (feedbackEffect) {
		case FEEDBACK_EFFECT_CUBE:
			effectFeedbackCubeRun();
		break;
		
		case FEEDBACK_EFFECT_DOTCUBE:
			effectDotcubeRun();
		break;

		case FEEDBACK_EFFECT_MOSAIK:
			effectMosaikRun();
		break;

		case FEEDBACK_EFFECT_SLIMECUBE:
			effectSlimecubeRun();
		break;
	}
}
