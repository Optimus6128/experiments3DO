#include "core.h"

#include "tools.h"
#include "engine_texture.h"
#include "mathutil.h"

texture *textures[TEXTURE_NUM];


void initTexture(int width, int height, int type, int bpp)
{
    int i, x, y, xc, yc, c;
	int size = width * height * (bpp / 8);
	texture *tex;

    tex = (texture*)malloc(sizeof(texture));
    textures[type] = tex;

    tex->width = width;
    tex->height = height;
    tex->bitmap = (ubyte*)malloc(size * sizeof(ubyte));
	tex->bpp = bpp;

    switch(type)
    {
        case TEXTURE_FLAT:
            for (i=0; i<size; i++)
                tex->bitmap[i] = 31;
        break;

		case TEXTURE_NOISE:
            for (i=0; i<size; i++)
                tex->bitmap[i] = getRand(1, 32767);
        break;

        case TEXTURE_XOR:
            i = 0;
            for (y=0; y<height; y++) {
                for (x=0; x<width; x++) {
                    tex->bitmap[i++] = x ^ y;
                }
            }
        break;

        case TEXTURE_GRID:
            i = 0;
            for (y=0; y<height; y++) {
                yc = y - (height >> 1);
                for (x=0; x<width; x++) {
                    xc = x - (width >> 1);
                    c = (xc * xc * xc * xc + yc * yc * yc * yc) >> 3;
                    if (c > 31) c = 31;
                    tex->bitmap[i++] = c;
                }
            }
        break;

        case TEXTURE_DRACUL:
            tex->bitmap = (ubyte*)dracul;
        break;
    }
}

texture *getTexture(int textureNum)
{
    return textures[textureNum];
}

/*void initTextures()
{
	initTexture(FB_WIDTH, FB_HEIGHT, TEXTURE_NOISE, 16);
	initTexture(16, 16, TEXTURE_FLAT, 8);
	initTexture(128, 128, TEXTURE_DRACUL, 16);
}*/
