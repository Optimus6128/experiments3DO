#include "types.h"
#include "main.h"

#include "system_graphics.h"
#include "input.h"

#include "tools.h"

#include "effect.h"

uint32 time;

static void initSystem()
{
	OpenMathFolio();
	OpenGraphicsFolio();
	InitEventUtility(1,0,LC_Observer);
	OpenAudioFolio();
}

static void initGame()
{
	effectInit();
}

static void initStuff()
{
	initSystem();
	initGraphics();
	initInput();

	initTools();

	initGame();
}

static void inputScript()
{
	if (isButtonPressedOnce(BUTTON_SELECT)) vsync = !vsync;
}

static void script()
{
	time = nframe;

	inputScript();

	effectRun();
}

static void mainLoop()
{
	for(;;)
	{
		updateJoypad();

		script();

		showFPS();
		//showAvailMem();
		//showDebugNums();

		displayScreen();

		++nframe;
	}
}


int main()
{
	initStuff();

	mainLoop();
}
