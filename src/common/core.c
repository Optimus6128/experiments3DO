#include "core.h"

#include "system_graphics.h"
#include "input.h"
#include "tools.h"


static void initSystem()
{
	OpenMathFolio();
	OpenGraphicsFolio();
	InitEventUtility(1,0,LC_Observer);
	OpenAudioFolio();
}

void initCore()
{
	initSystem();
	initGraphics();
	initInput();
	initTools();
}

void showSystemInfo(bool fps, bool mem)
{
	if (fps) showFPS();
	if (mem) showAvailMem();
}
