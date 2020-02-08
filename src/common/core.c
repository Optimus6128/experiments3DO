#include "core.h"

#include "system_graphics.h"
#include "input.h"
#include "tools.h"
#include "menu.h"

#include "mathutil.h"
#include "engine_main.h"

static bool showFps = false;
static bool showMem = false;
static bool defaultInput = false;
static bool menu = false;

static void initSystem()
{
	OpenMathFolio();
	OpenGraphicsFolio();
	InitEventUtility(1,0,LC_Observer);
	OpenAudioFolio();
}

void coreInit(void(*initFunc)(), int flags)
{
	initSystem();
	initGraphics();
	initInput();
	initTools();
	initMenu();

    initMathUtil();
	initEngine();

	initFunc();

	showFps = (flags & CORE_SHOW_FPS);
	showMem = (flags & CORE_SHOW_MEM);
	defaultInput = (flags & CORE_DEFAULT_INPUT);
}

static void defaultInputScript()
{
	if (defaultInput) {
		if (isButtonPressedOnce(BUTTON_SELECT)) toggleVsync();
		if (menu & isButtonPressedOnce(BUTTON_START)) showMenu();
	}
}

void displaySystemInfo()
{
	if (showFps) displayFPS();
	if (showMem) displayMem();
}

void coreRun(void(*mainLoopFunc)())
{
	while(true)
	{
		updateInput();

		defaultInputScript();
		
		mainLoopFunc();
		
		displaySystemInfo();

		displayScreen();
	}
}
