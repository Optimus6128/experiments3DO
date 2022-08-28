#include "core.h"

#include "effect_volumeScape.h"

#include "system_graphics.h"
#include "tools.h"
#include "input.h"
#include "mathutil.h"


#define HMAP_WIDTH 1024
#define HMAP_HEIGHT 1024
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)


uint8 *hmap;
uint8 *cmap;

static void loadHeightmap()
{
	hmap = AllocMem(HMAP_SIZE, MEMTYPE_ANY);
	//cmap = AllocMem(HMAP_SIZE/4, MEMTYPE_ANY);
}

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

	loadHeightmap();
}
