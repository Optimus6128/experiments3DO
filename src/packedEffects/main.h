#ifndef MAIN_H
#define MAIN_H

void effectPackedSpritesInit(void);
void effectPackedSpritesRun(void);

void effectPackedRainInit(void);
void effectPackedRainRun(void);

void effectPackedRadialInit(void);
void effectPackedRadialRun(void);

void effectPackedWaveInit(void);
void effectPackedWaveRun(void);


enum { EFFECT_PACKED_SPRITES, EFFECT_PACKED_RAIN, EFFECT_PACKED_RADIAL, EFFECT_PACKED_WAVE, EFFECTS_NUM };

void(*effectInitFunc[EFFECTS_NUM])() = { effectPackedSpritesInit, effectPackedRainInit, effectPackedRadialInit, effectPackedWaveInit };
void(*effectRunFunc[EFFECTS_NUM])() = { effectPackedSpritesRun, effectPackedRainRun, effectPackedRadialRun, effectPackedWaveRun };

char *effectName[EFFECTS_NUM] = { "packed sprites", "packed rain", "packed radial", "packed wave" };

#endif
