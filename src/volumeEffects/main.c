#include "core.h"

#include "effect.h"

int main()
{
	coreInit(effectInit, CORE_DEFAULT | CORE_INIT_3D_ENGINE);
	coreRun(effectRun);
}
