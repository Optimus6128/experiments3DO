#include "core.h"

#include "effect_volumeScape.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"
#include "mathutil.h"

void effectVolumeScapeInit()
{
}

static void updateFromInput()
{
	if (isJoyButtonPressed(JOY_BUTTON_A)) {
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
	}
	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_B)) {
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
	}

	if (isMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
	}
}

void effectVolumeScapeRun()
{
	updateFromInput();
}
