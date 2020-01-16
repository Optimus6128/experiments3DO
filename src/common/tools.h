#ifndef TOOLS_H
#define TOOLS_H

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define FONT_SIZE (FONT_WIDTH * FONT_HEIGHT)

#define FONTS_PAL_SIZE 32
#define FONTS_MAP_SIZE 256

#define MAX_STRING_LENGTH 64
#define NUM_FONTS 59

#define TEXT_ZOOM_SHR 8


extern int nframe;

void initFonts(void);
void initTimer(void);

void drawText(int xtp, int ytp, char *text);
void drawZoomedText(int xtp, int ytp, char *text, int zoom);
void drawNumber(int xtp, int ytp, int num);

void showFPS(void);
int getTicks(void);

void drawAvailMem(void);

void setPal(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, uint16 *pal);
void setPalWithFades(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, uint16* pal, int numFades, int r2, int g2, int b2);

#endif
