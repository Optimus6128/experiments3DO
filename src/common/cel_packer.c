#include "cel_packer.h"
#include "core.h"

typedef struct pixelRepeatsType
{
	int type;
	int pixelRepeats;
}pixelRepeatsType;

#define TEMP_BUFF_SIZE 262144
#define TEMP_LINE_SIZE 1024

static unsigned char *tempBuff;
static int *tempLine;

int packPercentage;

static int currentBit;

static bool packerIsReady = false;


static void extractPixelsToChunkyBytes(unsigned char *unpackedLine, int width, int bpp)
{
	int i;
	unsigned char *src8 = unpackedLine;
	uint16 *src16 = (uint16*)unpackedLine;
	int *dst = tempLine;

	switch(bpp) {
		case 1:
			for (i=0; i<width; i+=8) {
				const unsigned char b = *src8++;
				*dst++ = b >> 7;
				*dst++ = (b >> 6) & 1;
				*dst++ = (b >> 5) & 1;
				*dst++ = (b >> 4) & 1;
				*dst++ = (b >> 3) & 1;
				*dst++ = (b >> 2) & 1;
				*dst++ = (b >> 1) & 1;
				*dst++ = b & 1;
			}
		break;

		case 2:
			for (i=0; i<width; i+=4) {
				const unsigned char b = *src8++;
				*dst++ = b >> 6;
				*dst++ = (b >> 4) & 3;
				*dst++ = (b >> 2) & 3;
				*dst++ = b & 3;
			}
		break;

		case 4:
			for (i=0; i<width; i+=2) {
				const unsigned char b = *src8++;
				*dst++ = b >> 4;
				*dst++ = b & 15;
			}
		break;

		case 6:
		break;

		case 8:
			for (i=0; i<width; ++i) {
				*dst++ = *src8++;
			}
		break;

		case 16:
			for (i=0; i<width; ++i) {
				*dst++ = *src16++;
			}
		break;

		default:
		break;
	}
}

static bool isChunkyPixelTransparent(int pixel, int type, int bpp, uint16 *pal, int transparentColor)
{
	int palMax;
	int palBpp = bpp;
	if (palBpp > 5) palBpp = 5;
	palMax = (1 << palBpp) - 1;
	
	switch (type) {
	case CEL_TYPE_CODED:
		return (pal[pixel & palMax] == transparentColor);
		break;

	case CEL_TYPE_UNCODED:
		return (pixel == transparentColor);
		break;

	default:
		return true;
		break;
	}
}

static pixelRepeatsType* getPixelRepeats(int *src, int length, int bpp, int type, uint16 *pal, int transparentColor)
{
	static pixelRepeatsType prt;

	int x, pixel;

	pixel = *src++;

	for (x=1; x<length; ++x) {
		if (pixel != *src++) {
			break;
		}
	}

	if (isChunkyPixelTransparent(pixel, type, bpp, pal, transparentColor)) {
		prt.type = PACK_TRANSPARENT;
	} else {
		prt.type = PACK_PACKED;
		if (x < length) {
			if (x==1) {
				int count = 1;
				prt.type = PACK_LITERAL;
				--src;
				pixel = *src;
				if (!isChunkyPixelTransparent(pixel, type, bpp, pal, transparentColor)) {
					for (x=2; x<length; ++x) {
						if (count > 2) {
							x -= count;
							break;
						}
						if (pixel != *src++) {
							pixel = *src;
							count = 0;
						}
						++count;
					}
				}
			}
		}
	}
	prt.pixelRepeats = x;
	return &prt;
}

static void writeValueBits(int value, int bpp)
{
	int i;
	for (i = 0; i < bpp; ++i) {
		const int byteIndex = currentBit >> 3;
		const int writeBitIndex = 7 - (currentBit & 7);

		tempBuff[byteIndex] = (tempBuff[byteIndex] & ~(1 << writeBitIndex)) | (((value >> (bpp - i - 1)) & 1) << writeBitIndex);
		++currentBit;
	}
}

static void writeOneValue(int value, int bpp)
{
	if ((currentBit & 7) != 0){
		writeValueBits(value, bpp);
	} else {
		if (bpp==8) {
			tempBuff[currentBit >> 3] = value & 255; currentBit += 8;
		} else if (bpp==16) {
			*((uint16*)&tempBuff[currentBit >> 3]) = value & 65535; currentBit += 16;
		} else {
			writeValueBits(value, bpp);
		}
	}
}

static void writeManyValues(int *valuePtr, int bpp, int count)
{
	int i;

	for (i=0; i<count; ++i) {
		writeOneValue(*valuePtr, bpp);
		valuePtr++;
	}
}

static void writeEolToTempBuff()
{
	const int header = PACK_EOL << 6;
	writeOneValue(header, 8);
}

static void writePixelRepeatToTempBuff(int start, int bpp, pixelRepeatsType *prt)
{
	const int header = (prt->type << 6) | ((prt->pixelRepeats - 1) & 63);
	writeOneValue(header, 8);

	switch(prt->type) {
		case PACK_PACKED:
			writeOneValue(tempLine[start], bpp);
		break;

		case PACK_LITERAL:
			writeManyValues(&tempLine[start], bpp, prt->pixelRepeats);
		break;

		case PACK_TRANSPARENT:
		case PACK_EOL:
		default:
		break;
	}
}

static void padLineBitsAndWriteAddressOffset(int addressOffsetIndex, int bpp)
{
	const int leftoverBits = currentBit & 31;
	const int remainingBits = (32 - leftoverBits) & 31;

	int totalWords, wordOffset;

	writeValueBits(0, remainingBits);

	totalWords = (currentBit >> 5) - (addressOffsetIndex >> 2);
	while (totalWords < 2) {
		*((uint32*)&tempBuff[currentBit >> 3]) = 0;
		currentBit += 32;
		++totalWords;
	}

	wordOffset = totalWords - 2;
	if (bpp >= 8)
		*((uint16*)&tempBuff[addressOffsetIndex]) = wordOffset & 1023;
	else
		tempBuff[addressOffsetIndex] = wordOffset & 255;
}


void initCelPackerEngine()
{
	tempBuff = (unsigned char*)AllocMem(TEMP_BUFF_SIZE, MEMTYPE_ANY);
	tempLine = (int*)AllocMem(TEMP_LINE_SIZE * 4, MEMTYPE_ANY);
	packerIsReady = true;
}

void deinitCelPackerEngine()
{
	FreeMem(tempBuff, TEMP_BUFF_SIZE);
	FreeMem(tempLine, TEMP_LINE_SIZE * 4);
	packerIsReady = false;
}

unsigned char* createPackedDataFromUnpackedBmp(int width, int height, int bpp, int type, uint16 *pal, unsigned char *unpackedBmp, int transparentColor)
{
	int x, y;
	int countBytes = 0;
	unsigned char *packedData = NULL;

	if (!packerIsReady) initCelPackerEngine();

	currentBit = 0;

	for (y=0; y<height; ++y) {
		unsigned char *src = &unpackedBmp[(y * width * bpp) >> 3];

		if (bpp >= 8)
			currentBit += 16;
		else
			currentBit += 8;

		extractPixelsToChunkyBytes(src, width, bpp);

		x = 0;
		while (x < width) {
			pixelRepeatsType *prt;
			int length = width - x;
			if (length > 64) length = 64;
			prt = getPixelRepeats(&tempLine[x], length, bpp, type, pal, transparentColor);
			writePixelRepeatToTempBuff(x, bpp, prt);
			x += prt->pixelRepeats;
		}
		writeEolToTempBuff();

		padLineBitsAndWriteAddressOffset(countBytes, bpp);
		countBytes = currentBit >> 3;
	}

	packedData = (unsigned char*)AllocMem(countBytes, MEMTYPE_ANY);
	memcpy(packedData, tempBuff, countBytes);

	packPercentage = (countBytes * 100) / ((width * height * bpp) >> 3);

	return packedData;
}
