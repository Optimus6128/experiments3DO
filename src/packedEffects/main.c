#include "core.h"
#include "tools.h"

#include "main.h"

int main()
{
	const int effectIndex = runEffectSelector(effectName, EFFECTS_NUM);

	coreInit(effectInitFunc[effectIndex], CORE_DEFAULT);// | CORE_SHOW_MEM);
	coreRun(effectRunFunc[effectIndex]);
}
