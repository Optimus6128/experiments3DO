#include "types.h"
#include "core.h"

#include "system_graphics.h"
#include "input.h"

#include "tools.h"
#include "effect.h"


static void init()
{
	initCore();

	effectInit();
}

static void inputScript()
{
	if (isButtonPressedOnce(BUTTON_SELECT)) toggleVsync();
}

static void script()
{
	inputScript();

	effectRun();
}

static void mainLoop()
{
	while(true)
	{
		updateJoypad();

		script();
		
		showSystemInfo(true, false);

		displayScreen();
	}
}

int main()
{
	init();

	mainLoop();
}
