#include "effect.h"
#include "effect_packedSprites.h"
#include "effect_packedRain.h"

enum { EFFECT_PACKED_SPRITES, EFFECT_PACKED_RAIN };

static int packedEffect = EFFECT_PACKED_SPRITES;

void effectInit()
{
	switch (packedEffect) {
		case EFFECT_PACKED_SPRITES:
			effectPackedSpritesInit();
		break;
		
		case EFFECT_PACKED_RAIN:
			effectPackedRainInit();
		break;
	}
}

void effectRun()
{
	switch (packedEffect) {
		case EFFECT_PACKED_SPRITES:
			effectPackedSpritesRun();
		break;
		
		case EFFECT_PACKED_RAIN:
			effectPackedRainRun();
		break;
	}
}
