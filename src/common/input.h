#ifndef INPUT_H
#define INPUT_H

enum {
	BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT,
	BUTTON_A, BUTTON_B, BUTTON_C,
	BUTTON_LPAD, BUTTON_RPAD,
	BUTTON_SELECT, BUTTON_START, BUTTONS_NUM
};


void initInput(void);
void updateInput(void);

bool isButtonPressed(int buttonId);
bool isButtonPressedOnce(int buttonId);

#endif
