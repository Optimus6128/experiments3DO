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

#define MAX_LOADING_BARS 8
static CCB *loadingBarCel[MAX_LOADING_BARS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

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

static void renderLoadingBars()
{
	int i;

	for (i=0; i<MAX_LOADING_BARS; ++i) {
		if (loadingBarCel[i]) {
			drawCels(loadingBarCel[i]);
		}
	}

	displayScreen();
}

static void freeLoadingBars()
{
	int i;
	for (i=0; i<MAX_LOADING_BARS; ++i) {
		if (loadingBarCel[i]) {
			DeleteCel(loadingBarCel[i]);
		}
	}
}

void updateLoadingBar(int bar, int status, int max)
{
	int length;

	if (bar < 0 || bar >= MAX_LOADING_BARS || status<=0) return;

	if (max < status) max = status;
	length = (status * SCREEN_WIDTH) / max;

	if (!loadingBarCel[bar]) {
		loadingBarCel[bar] = CreateBackdropCel(SCREEN_WIDTH, 8, 0x7FFF, 100);
	}

	loadingBarCel[bar]->ccb_XPos = 0;
	loadingBarCel[bar]->ccb_YPos = (2 + bar * 10) << 16;
	loadingBarCel[bar]->ccb_HDX = length << 20;

	renderLoadingBars();
}

void coreInit(void(*initFunc)(), uint32 flags)
{
	bool mustInitEngine3D = (flags & CORE_INIT_3D_ENGINE) != 0;
	bool mustInitEngine3D_soft = (flags & CORE_INIT_3D_ENGINE_SOFT) != 0;
	
	initSystem();
	initGraphicsOptions(flags);
	initInput();
	initTools();
	initMenu();

	if (mustInitEngine3D || mustInitEngine3D_soft) {
		initEngine(mustInitEngine3D_soft);
	}

	if (initFunc) {
		initFunc();
	}

	showFps = (flags & CORE_SHOW_FPS);
	showMem = (flags & CORE_SHOW_MEM);
	showBuffers = (flags & CORE_SHOW_BUFFERS);
	
	defaultInput = (flags & CORE_DEFAULT_INPUT);

	freeLoadingBars();
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
