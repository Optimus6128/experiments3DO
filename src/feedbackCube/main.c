#include "core.h"

#include "effect.h"

int main()
{
	coreInit(effectInit, CORE_DEFAULT | CORE_VRAM_DOUBLEBUFFER | CORE_OFFSCREEN_BUFFERS(2));
	coreRun(effectRun);
}
