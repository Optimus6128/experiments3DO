#include "core.h"

#include "effect_fliAnimTest.h"

#include "system_graphics.h"
#include "sprite_engine.h"
#include "anim_fli.h"
#include "tools.h"
#include "input.h"

#define FLI_BMP_WIDTH 320
#define FLI_BMP_HEIGHT 200

static Sprite *fliSpr;
static uint16 *fliBmp;

AnimFLI *fli;


void effectFliAnimTestInit()
{
	fliBmp = (uint16*)AllocMem(2 * FLI_BMP_WIDTH * FLI_BMP_HEIGHT, MEMTYPE_ANY);
	fliSpr = newSprite(FLI_BMP_WIDTH, FLI_BMP_HEIGHT, 16, CEL_TYPE_UNCODED, NULL, (uint8*)fliBmp);

	setSpritePosition(fliSpr, 0, (SCREEN_HEIGHT - FLI_BMP_HEIGHT) / 2);

	fli = newAnimFLI("data/wavelogo.fli");
	//fli = newAnimFLI("data/rrhood.fli");
	//fli = newAnimFLI("data/acid.fli");

	FLIload(fli);
}

void effectFliAnimTestRun()
{
	FLIplayNextFrame(fli);
	FLIshow(fli, fliBmp);

	drawSprite(fliSpr);
}
