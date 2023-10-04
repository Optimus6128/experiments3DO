#include "core.h"

#include "system_graphics.h"
#include "tools.h"

#include "mathutil.h"
#include "celutils.h"
#include "cel_helpers.h"

#include "sprite_engine.h"
#include "input.h"

Sprite *fenceSpr;
Sprite *grassSpr;
Sprite *balconSpr;
Sprite *threedoSpr;

static bool showBg = true;
static bool show3DO = false;
static bool showBalcon1 = true;
static bool showBalcon2 = true;
static bool showGrass = true;
static bool showFence = true;
static bool blend3DO = false;

static void setBlend(CCB *cel, bool enabled)
{
	if (enabled) {
		cel->ccb_PIXC = CEL_BLEND_AVERAGE;
	} else {
		cel->ccb_PIXC = CEL_BLEND_OPAQUE;
	}
}

void effectParallaxInit()
{
	const int superClippingFlags = (CCB_ACSC | CCB_ALSC);

	setBackgroundColor(0x1F1F1F1F);
	loadAndSetBackgroundImage("data/flare.img", getBackBuffer());

	fenceSpr = loadSpriteCel("data/fence.cel");
	grassSpr = loadSpriteCel("data/grass.cel");
	balconSpr = loadSpriteCel("data/balcon.cel");
	threedoSpr = loadSpriteCel("data/3DO.cel");

	fenceSpr->cel->ccb_Flags |= superClippingFlags;
	grassSpr->cel->ccb_Flags |= superClippingFlags;
	balconSpr->cel->ccb_Flags |= superClippingFlags;
	threedoSpr->cel->ccb_Flags |= superClippingFlags;
}

static void inputScript()
{
	if (isJoyButtonPressedOnce(JOY_BUTTON_LEFT)) {
		showBalcon1 = !showBalcon1;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_UP)) {
		showBg = !showBg;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_RIGHT)) {
		showBalcon2 = !showBalcon2;
	}
	if (isJoyButtonPressedOnce(JOY_BUTTON_DOWN)) {
		showGrass = !showGrass;
	}

	if (isJoyButtonPressedOnce(JOY_BUTTON_A)) {
		show3DO = !show3DO;
	}
	if (isJoyButtonPressedOnce(JOY_BUTTON_B)) {
		blend3DO = !blend3DO;
	}
	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		showFence = !showFence;
	}
}

void effectParallaxRun()
{
	static int threedoY = -240;
	static int threedoD = -1;

	const int t = getTicks();
	const int grassT = -((t / 8) & 511);
	const int balconT1 = 256 - ((t / 12) & 511);
	const int balconT2 = 256 - ((t / 16) & 511);

	inputScript();

	if (showBg) {
		switchToSPORTimage();
	} else {
		switchToSPORTwrite();
	}

	if (show3DO) {
		setBlend(threedoSpr->cel, blend3DO);
		setSpritePosition(threedoSpr, 0,threedoY);
		drawSprite(threedoSpr);

		threedoY += threedoD;
		if (threedoY<=-240) threedoD = 1;
		if (threedoY>=0) threedoD = -1;
	}

	if (showBalcon2) {
		setSpritePositionZoomRotate(balconSpr, 160+balconT2,120, 128, -64<<8);
		drawSprite(balconSpr);
	}
	if (showBalcon1) {
		setSpritePositionZoomRotate(balconSpr, 160+balconT1,120, 256, -64<<8);
		drawSprite(balconSpr);
	}

	if (showGrass) {
		setSpritePosition(grassSpr, grassT, 112);
		drawSprite(grassSpr);
		setSpritePosition(grassSpr, grassT+512, 112);
		drawSprite(grassSpr);
	}

	if (showFence) {
		const int x1 = -160 + (SinF16(t<<12) >> 9);
		const int y1 = -120 + (SinF16(t<<11) >> 10);
		setSpritePosition(fenceSpr, x1,y1);
		drawSprite(fenceSpr);
	}
}
