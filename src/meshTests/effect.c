#include "effect.h"
#include "effect_pyramids.h"
#include "effect_soft.h"

static enum { MESH_EFFECT_PYRAMIDS, MESH_EFFECT_SOFT };

static int meshEffect = MESH_EFFECT_SOFT;

void effectInit()
{
	switch (meshEffect) {
		case MESH_EFFECT_PYRAMIDS:
			effectPyramidsInit();
		break;
		
		case MESH_EFFECT_SOFT:
			effectSoftInit();
		break;
	}
}

void effectRun()
{
	switch (meshEffect) {
		case MESH_EFFECT_PYRAMIDS:
			effectPyramidsRun();
		break;
		
		case MESH_EFFECT_SOFT:
			effectSoftRun();
		break;
	}
}
