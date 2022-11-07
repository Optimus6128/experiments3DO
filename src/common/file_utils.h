#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "types.h"
#include "core.h"

#define SHORT_ENDIAN_FLIP(v) (uint16)( (((v) >> 8) & 255) | ((v) << 8) )
#define LONG_ENDIAN_FLIP(v) (uint32)( (((v) >> 24) & 0x000000FF) | (((v) >> 8) & 0x0000FF00) | (((v) << 8) & 0x00FF0000) | ((v) << 24) )

void openFileStream(char *path);
void closeFileStream(void);

void moveFilePointer(int offset);
void moveFilePointerRelative(int offset);

char *readBytesFromFile(int offset, int size);
char *readSequentialBytesFromFile(int size);

void readBytesFromFileAndStore(char *path, int offset, int size, uint8 *dst);

#endif
