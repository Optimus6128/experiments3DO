#ifndef TOOLS_H
#define TOOLS_H

#define TINY_FONT_WIDTH 8
#define TINY_FONT_HEIGHT 8
#define TINY_FONT_SIZE (TINY_FONT_WIDTH * TINY_FONT_HEIGHT)

#define TINY_FONTS_PAL_SIZE 32
#define TINY_FONTS_MAP_SIZE 256

#define MAX_STRING_LENGTH 64
#define NUM_TINY_FONTS 59

#define TEXT_ZOOM_SHR 8

#define CLAMP(value,min,max) if ((value)<(min)) (value)=(min); if ((value)>(max)) (value)=(max);
#define CLAMP_LEFT(value,min) if ((value)<(min)) (value)=(min);
#define CLAMP_RIGHT(value,max) if ((value)>(max)) (value)=(max);


#include "graphics.h"


void initTools(void);

void drawText(int xtp, int ytp, char *text);
void drawTextX2(int xtp, int ytp, char *text);
void drawZoomedText(int xtp, int ytp, char *text, int zoom);
void drawNumber(int xtp, int ytp, int num);
void setTextColor(uint16 color);

void displayFPS(void);
void displayMem(void);
void displayBuffers(void);

int getTicks(void);

int runEffectSelector(char **str, int size);

void setPal(int c, int r, int g, int b, uint16* pal);
void setPalGradient(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, uint16* pal);
void setPalGradientFromPrevIndex(int c0, int c1, int r1, int g1, int b1, uint16* pal);
uint16 shadeColor(uint16 c, int shade);	// dark=0, bright=256
void drawBorderEdges(int posX, int posY, int width, int height);

void printDebugNum(int num);
void displayDebugNums(bool resetAfter);

#endif
