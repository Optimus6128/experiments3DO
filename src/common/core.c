#include "core.h"

#include "system_graphics.h"
#include "input.h"
#include "tools.h"
#include "menu.h"

#include "mathutil.h"
#include "engine_main.h"

static bool showFps = false;
static bool showMem = false;
static bool showBuffers = false;
static bool defaultInput = false;
static bool menu = false;

static void initSystem()
{
	static bool systemIsInit = false;

	if (!systemIsInit) {
		OpenMathFolio();
		OpenGraphicsFolio();
		InitEventUtility(1,0,LC_Observer);
		OpenAudioFolio();

		systemIsInit = true;
	}
}

static void initGraphicsOptions(uint32 flags)
{
	const bool horizontalAntialiasing = flags & CORE_HORIZONTAL_ANTIALIASING;
	const bool verticalAntialiasing = flags & CORE_VERTICAL_ANTIALIASING;

	const uint32 numBuffersMax = (1 << MAX_NUMBUFFER_BITS) - 1;
	uint32 numVramBuffers = (flags >> VRAM_NUMBUFFERS_BITS_START) & numBuffersMax;
	uint32 numOffscreenBuffers = (flags >> OFFSCREEN_NUMBUFFERS_BITS_START) & numBuffersMax;

	initGraphics(numVramBuffers, numOffscreenBuffers, horizontalAntialiasing, verticalAntialiasing);

	setVsync(!(flags & CORE_NO_VSYNC));
	setClearFrame(!(flags & CORE_NO_CLEAR_FRAME));
}

void coreInit(void(*initFunc)(), uint32 flags)
{
	initSystem();
	initGraphicsOptions(flags);
	initInput();
	initTools();
	initMenu();

	initMathUtil();
	initEngine();

	if (initFunc) initFunc();

	showFps = (flags & CORE_SHOW_FPS);
	showMem = (flags & CORE_SHOW_MEM);
	showBuffers = (flags & CORE_SHOW_BUFFERS);
	
	defaultInput = (flags & CORE_DEFAULT_INPUT);
}

static void defaultInputScript()
{
	if (defaultInput) {
		if (isJoyButtonPressedOnce(JOY_BUTTON_SELECT)) toggleVsync();
		if (menu & isJoyButtonPressedOnce(JOY_BUTTON_START)) showMenu();
	}
}

void displaySystemInfo()
{
	if (showFps) displayFPS();
	if (showMem) displayMem();
	if (showBuffers) displayBuffers();
}

void setShowFps(bool on)
{
	showFps = on;
}

void setShowMem(bool on)
{
	showMem = on;
}

void setShowBuffers(bool on)
{
	showBuffers = on;
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
