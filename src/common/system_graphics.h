#ifndef SYSTEM_GRAPHICS_H
#define SYSTEM_GRAPHICS_H

#include "core.h"


#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_SIZE_IN_BYTES (SCREEN_WIDTH * SCREEN_HEIGHT * 2)

#define NUM_SCREEN_PAGES 2
#define NUM_BUFFER_PAGES 2
#define NUM_TOTAL_PAGES (NUM_SCREEN_PAGES + NUM_BUFFER_PAGES)

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
uint16 *getVramBuffer(void);
uint16 *getBackBuffer(void);

void switchBuffer(bool on);
void setBuffer(uint32 num);

#endif
