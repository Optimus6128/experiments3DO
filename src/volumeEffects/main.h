#ifndef MAIN_H
#define MAIN_H

void effectVolumeCubeInit(void);
void effectVolumeCubeRun(void);

void effectVolumeScapeInit(void);
void effectVolumeScapeRun(void);

void effectVolumeScapeGradientInit(void);
void effectVolumeScapeGradientRun(void);


enum { EFFECT_VOLUME_CUBE, EFFECT_VOLUME_SCAPE, EFFECT_VOLUME_SCAPE_GRADIENT, EFFECTS_NUM };

void(*effectInitFunc[EFFECTS_NUM])() = { effectVolumeCubeInit, effectVolumeScapeInit, effectVolumeScapeGradientInit };
void(*effectRunFunc[EFFECTS_NUM])() = { effectVolumeCubeRun, effectVolumeScapeRun, effectVolumeScapeGradientRun };

char *effectName[EFFECTS_NUM] = { "volume cube", "volume scape", "volume scape gradient" };

#endif
