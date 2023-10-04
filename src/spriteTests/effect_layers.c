#include "core.h"

#include "system_graphics.h"
#include "tools.h"

#include "mathutil.h"
#include "cel_helpers.h"
#include "celutils.h"

#include "input.h"


static CCB *bg1Cel, *bg2Cel, *bg3Cel;
static void *bg1data, *bg2data, *bg3data;

static bool show0 = true;
static bool show1 = false;
static bool show2 = true;
static bool show3 = true;

static bool blend1 = true;
static bool blend2 = true;
static bool blend3 = true;


static void setBlend(CCB *cel, bool enabled)
{
	if (enabled) {
		cel->ccb_PIXC = CEL_BLEND_ADDITIVE;
	} else {
		cel->ccb_PIXC = CEL_BLEND_OPAQUE;
	}
}

static void updateLayers()
{
	if (show0) {
		switchToSPORTimage();
	} else {
		switchToSPORTwrite();
	}

	setCelFlags(bg1Cel, CCB_SKIP, !show1);
	setCelFlags(bg2Cel, CCB_SKIP, !show2);
	setCelFlags(bg3Cel, CCB_SKIP, !show3);

	setBlend(bg1Cel, blend1);
	setBlend(bg2Cel, blend2);
	setBlend(bg3Cel, blend3);
}

void effectLayersInit()
{
	loadAndSetBackgroundImage("data/background.img", getBackBuffer());

	bg1Cel = LoadCel("data/lamepic.cel", MEMTYPE_ANY);
	bg2Cel = LoadCel("data/planet.cel", MEMTYPE_ANY);
	bg3Cel = LoadCel("data/moon.cel", MEMTYPE_ANY);

	bg1data = getCelBitmap(bg1Cel);
	bg2data = getCelBitmap(bg2Cel);
	bg3data = getCelBitmap(bg3Cel);

	linkCel(bg1Cel, bg2Cel);
	linkCel(bg2Cel, bg3Cel);
}

static void inputScript()
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_LEFT)) {
		show1 = !show1;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_UP)) {
		show2 = !show2;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_RIGHT)) {
		show3 = !show3;
	}
	if (isJoyButtonPressedOnce(JOY_BUTTON_DOWN)) {
		show0 = !show0;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_A)) {
		blend1 = !blend1;
	}
	if (isJoyButtonPressedOnce(JOY_BUTTON_B)) {
		blend2 = !blend2;
	}
	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		blend3 = !blend3;
	}

	if (isJoyButtonPressed(JOY_BUTTON_LPAD)) {
		blend1 = blend2 = blend3 = false;
	}
	if (isJoyButtonPressed(JOY_BUTTON_RPAD)) {
		blend1 = blend2 = blend3 = true;
	}
}

static void drawTextDisplay()
{
	if (show0) drawText(16,200, "BG SPORT");
	if (show1) drawText(16,208, "BG 1");
	if (show2) drawText(16,216, "BG 2");
	if (show3) drawText(16,224, "BG 3");
}


void effectLayersRun()
{
	const int t = getTicks();

	//const int x1 = 160 + (SinF16(t<<12) >> 9);
	//const int y1 = 120 + (SinF16(t<<11) >> 10);
	const int x2 = 160 + (SinF16(t<<12) >> 9);
	const int y2 = 120 + (SinF16(t<<11) >> 10);
	//const int x3 = 160 + (SinF16(t<<13) >> 9);
	//const int y3 = 120 + (SinF16(t<<12) >> 10);

	//updateWindowCel(x1,y1, 320,240, bg1data, bg1Cel);
	updateWindowCel(x2,y2, 320,240, bg2data, bg2Cel);
	//updateWindowCel(x3,y3, 320,240, bg3data, bg3Cel);	// this doesn't work for now, possibly memory is full after adding the anim_fli functions or I broke something

	inputScript();

	updateLayers();
	drawCels(bg1Cel);

	drawTextDisplay();
}
