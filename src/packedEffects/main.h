#ifndef MAIN_H
#define MAIN_H

void effectPackedSpritesInit(void);
void effectPackedSpritesRun(void);

void effectPackedRainInit(void);
void effectPackedRainRun(void);

void effectPackedRadialInit(void);
void effectPackedRadialRun(void);


enum { EFFECT_PACKED_SPRITES, EFFECT_PACKED_RAIN, EFFECT_PACKED_RADIAL, EFFECTS_NUM };

void(*effectInitFunc[EFFECTS_NUM])() = { effectPackedSpritesInit, effectPackedRainInit, effectPackedRadialInit };
void(*effectRunFunc[EFFECTS_NUM])() = { effectPackedSpritesRun, effectPackedRainRun, effectPackedRadialRun };

char *effectName[EFFECTS_NUM] = { "packed sprites", "packed rain", "packed radial" };

#endif
