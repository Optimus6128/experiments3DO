#include "core.h"

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


static char *filenames[] = { "acid.fli", "apple.fli", "birdsh01.fli", "clowns.fli", "loom.fli", "robo01.fli", "rrhood.fli", "wavelogo.fli", "watrfall.fli", "cycle.fli", "catnap.fli", "robotrk.fli" };

void effectFliAnimTestInit()
{
	static char fliFilePath[32];

	fliBmp = (uint16*)AllocMem(2 * FLI_BMP_WIDTH * FLI_BMP_HEIGHT, MEMTYPE_ANY);
	fliSpr = newSprite(FLI_BMP_WIDTH, FLI_BMP_HEIGHT, 16, CEL_TYPE_UNCODED, NULL, (unsigned char*)fliBmp);
	fliSpr->cel->ccb_Flags |= CCB_BGND;

	//setSpritePosition(fliSpr, 0, (SCREEN_HEIGHT - FLI_BMP_HEIGHT) / 2);

	sprintf(fliFilePath, "data/%s", filenames[11]);
	fli = newAnimFLI(fliFilePath, fliBmp);

	FLIload(fli, false);
}

void effectFliAnimTestRun()
{
	FLIplayNextFrame(fli);

	drawSprite(fliSpr);

	//displayDebugNums(true);
}
