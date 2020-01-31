#include "types.h"
#include "core.h"
#include "input.h"

// order must correspond to enum order
static int buttonHwIDs[BUTTONS_NUM] = { ControlUp, ControlDown, ControlLeft, ControlRight, ControlA, ControlB, ControlC, ControlLeftShift, ControlRightShift, ControlX, ControlStart };

static bool buttonPressed[BUTTONS_NUM];
static bool buttonPressedOnce[BUTTONS_NUM];


void initInput()
{
	int i;
	for (i=0; i<BUTTONS_NUM; ++i) {
		buttonPressed[i] = false;
		buttonPressedOnce[i] = false;
	}
}

void updateJoypad()
{
	int i, joybits;

	ControlPadEventData cpaddata;
	cpaddata.cped_ButtonBits=0;
	GetControlPad(1,0,&cpaddata);

	joybits = cpaddata.cped_ButtonBits;

	for (i=0; i<BUTTONS_NUM; ++i) {
		if (joybits & buttonHwIDs[i]) {
			buttonPressedOnce[i] = !buttonPressed[i];
			buttonPressed[i] = true;
		} else {
			buttonPressed[i] = false;
			buttonPressedOnce[i] = false;
		}
	}
}

bool isButtonPressed(int buttonId)
{
	if (buttonId < 0 || buttonId >= BUTTONS_NUM) return false;
	return buttonPressed[buttonId];
}

bool isButtonPressedOnce(int buttonId)
{
	if (buttonId < 0 || buttonId >= BUTTONS_NUM) return false;
	return buttonPressedOnce[buttonId];
}
