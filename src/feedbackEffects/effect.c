#include "effect.h"
#include "effect_feedbackCube.h"
#include "effect_feedbackOther.h"

enum { FEEDBACK_EFFECT_CUBE, FEEDBACK_EFFECT_OTHER };

static int feedbackEffect = FEEDBACK_EFFECT_OTHER;

void effectInit()
{
	switch (feedbackEffect) {
		case FEEDBACK_EFFECT_CUBE:
			effectFeedbackCubeInit();
		break;
		
		case FEEDBACK_EFFECT_OTHER:
			effectFeedbackOtherInit();
		break;
	}
}

void effectRun()
{
	switch (feedbackEffect) {
		case FEEDBACK_EFFECT_CUBE:
			effectFeedbackCubeRun();
		break;
		
		case FEEDBACK_EFFECT_OTHER:
			effectFeedbackOtherRun();
		break;
	}
}
