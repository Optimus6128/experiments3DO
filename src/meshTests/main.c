#include "core.h"

#include "effect.h"

int main()
{
	coreInit(effectInit, CORE_DEFAULT | CORE_NO_CLEAR_FRAME);
	coreRun(effectRun);
}
