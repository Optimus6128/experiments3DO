#include "core.h"

#include "effect.h"

int main()
{
	coreInit(effectInit, CORE_DEFAULT);
	coreRun(effectRun);
}
