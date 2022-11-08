#ifndef SYSTEM_GRAPHICS_H
#define SYSTEM_GRAPHICS_H

#include "core.h"


#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_SIZE_IN_BYTES (SCREEN_WIDTH * SCREEN_HEIGHT * 2)

#define DEFAULT_VRAM_BUFFERS_NUM 2
#define DEFAULT_OFFSCREEN_BUFFERS_NUM 1

void initGraphics(uint32 numVramBuffers, uint32 numOffscreenBuffers, bool horizontalAntialiasing, bool verticalAntialiasing);
void deInitGraphics(void);
void displayScreen(void);
void debugUpdate(void);
void drawCels(CCB *cels);

void loadAndSetBackgroundImage(char *path, void *screenBuffer);	// if screenBuffer is NULL, one will automatically allocated in VRAM
void setBackgroundColor(int color);
void clearAllBuffers(void);
void clearBackBuffer(void);
void drawPixel(int px, int py, uint16 c);
void drawThickPixel(int px, int py, uint16 c);	// coordinates map in 160*120 thick 2*2 pixels
int getFrameNum(void);

void setClearFrame(bool on);
void setVsync(bool on);
void toggleVsync(void);

uint16 *getVramBuffer(void);
uint16 *getBackBuffer(void);
uint16 *getBackBufferByIndex(int index);

uint32 getNumVramBuffers(void);
uint32 getNumOffscreenBuffers(void);

void switchRenderToBuffer(bool on);
void setRenderBuffer(uint32 num);

void switchToSPORTwrite(void);
void switchToSPORTimage(void);

#endif
