#include <math.h>

#include "core.h"

#include "system_graphics.h"
#include "tools.h"

#include "mathutil.h"
#include "input.h"

#include "raytrace.h"


void effectRaytraceInit()
{
	loadAndSetBackgroundImage("data/background.img", getBackBuffer());

	raytraceInit();
}

void effectRaytraceRun()
{
	raytraceRun(getTicks());
}
