#ifndef MAIN_H
#define MAIN_H

void effectSpritesGeckoInit(void);
void effectSpritesGeckoRun(void);

void effectLayersInit(void);
void effectLayersRun(void);

void effectParallaxInit(void);
void effectParallaxRun(void);

void effectJuliaInit(void);
void effectJuliaRun(void);

void effectWaterInit(void);
void effectWaterRun(void);

void effectSphereInit(void);
void effectSphereRun(void);

void effectFliAnimTestInit(void);
void effectFliAnimTestRun(void);

void effectAmvInit(void);
void effectAmvRun(void);


enum { EFFECT_SPRITES_GECKO, EFFECT_LAYERS, EFFECT_PARALLAX, EFFECT_JULIA, EFFECT_WATER, EFFECT_SPHERE, EFFECT_FLI_ANIM_TEST, EFFECT_AMV, EFFECTS_NUM };

void(*effectInitFunc[EFFECTS_NUM])() = { effectSpritesGeckoInit, effectLayersInit, effectParallaxInit, effectJuliaInit, effectWaterInit, effectSphereInit, effectFliAnimTestInit, effectAmvInit };
void(*effectRunFunc[EFFECTS_NUM])() = { effectSpritesGeckoRun, effectLayersRun, effectParallaxRun, effectJuliaRun, effectWaterRun, effectSphereRun, effectFliAnimTestRun, effectAmvRun };

char *effectName[EFFECTS_NUM] = { "1920 gecko sprites", "background layers", "parallax tests", "julia fractal", "water ripples", "sphere mapping", "fli animation test", "AMV bits" };

#endif
