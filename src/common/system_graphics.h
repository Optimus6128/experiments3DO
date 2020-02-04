#ifndef SYSTEM_GRAPHICS_H
#define SYSTEM_GRAPHICS_H

#include "core.h"


#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_SIZE_IN_BYTES (SCREEN_WIDTH * SCREEN_HEIGHT * 2)

#define NUM_SCREEN_PAGES 2

void initGraphics(void);
void displayScreen(void);
void debugUpdate(void);
void drawCels(CCB *cels);

void loadAndSetBackgroundImage(char *path);
void setBackgroundColor(int color);
void drawPixel(int px, int py, uint16 c);
int getFrameNum(void);

void setVsync(bool on);
void toggleVsync(void);

#endif
