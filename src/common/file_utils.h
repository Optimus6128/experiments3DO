#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "types.h"
#include "core.h"

#define SHORT_ENDIAN_FLIP(v) (uint16)( (((v) >> 8) & 255) | ((v) << 8) )
#define LONG_ENDIAN_FLIP(v) (uint32)( (((v) >> 24) & 0x000000FF) | (((v) >> 8) & 0x0000FF00) | (((v) << 8) & 0x00FF0000) | ((v) << 24) )

Stream *openFileStream(char *path);
void closeFileStream(Stream *CDstream);

void moveFileStreamPointer(int offset, Stream *CDstream);
void moveFileStreamPointerRelative(int offset, Stream *CDstream);

void readBytesFromFileStream(int offset, int size, void *dst, Stream *CDstream);
void readSequentialBytesFromFileStream(int size, void *dst, Stream *CDstream);

void readBytesFromFileAndClose(char *path, int offset, int size, void *dst);

#endif
