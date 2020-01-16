#include "cel_packer.h"
#include "main_includes.h"

static ubyte tempBuff[262144];
static int tempLine[1024];

int packPercentage;

typedef struct pixelRepeatsType
{
	int type;
	int pixelRepeats;
}pixelRepeatsType;

static int currentBit;


static void extractPixelsToChunkyBytes(ubyte *unpackedLine, int width, int bpp)
{
	int i;
	ubyte *src8 = unpackedLine;
	uint16 *src16 = (uint16*)unpackedLine;
	int *dst = tempLine;

	switch(bpp) {
		case 1:
		break;

		case 2:
		break;

		case 4:
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

static bool isChunkyPixelTransparent(int pixel, int type, uint16 *pal)
{
	switch (type) {
	case CREATECEL_CODED:
		return (pal[pixel] == 0);
		break;

	case CREATECEL_UNCODED:
		return (pixel == 0);
		break;

	default:
		return true;
		break;
	}
}

static pixelRepeatsType* getPixelRepeats(int *src, int length, int bpp, int type, uint16 *pal)
{
	static pixelRepeatsType prt;

	int x, pixel;

	pixel = *src++;

	for (x=1; x<length; ++x) {
		if (pixel != *src++) {
			break;
		}
	}

	if (isChunkyPixelTransparent(pixel, type, pal)) {
		prt.type = PACK_TRANSPARENT;
	} else {
		prt.type = PACK_PACKED;
		if (x < length) {
			if (x==1) {
				int count = 1;
				prt.type = PACK_LITERAL;
				--src;
				pixel = *src;
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
	prt.pixelRepeats = x;
	return &prt;
}

static void writeValueBits(int value, int bpp)
{
	int i;

	for (i = 0; i < bpp; ++i) {
		const int byteIndex = currentBit >> 3;
		const int writeBitIndex = currentBit & 7;

		tempBuff[byteIndex] |= (((value >> (bpp - i - 1)) & 1) << writeBitIndex);
		++currentBit;
	}
}

static void writeOneValue(int value, int bpp)
{
	if (bpp==8) {
		tempBuff[currentBit >> 3] = value & 255; currentBit += 8;
	} else if (bpp==16) {
		*((uint16*)&tempBuff[currentBit >> 3]) = value & 65535; currentBit += 16;
	} else {
		writeValueBits(value, bpp);
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

ubyte* createPackedDataFromUnpackedBmp(int width, int height, int bpp, int type, uint16 *pal, ubyte *unpackedBmp)
{
	int x, y;
	int countBytes = 0;
	ubyte *packedData = NULL;

	currentBit = 0;

	for (y=0; y<height; ++y) {
		ubyte *src = &unpackedBmp[(y * width * bpp) >> 3];

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
			prt = getPixelRepeats(&tempLine[x], length, bpp, type, pal);
			writePixelRepeatToTempBuff(x, bpp, prt);
			x += prt->pixelRepeats;
		}

		padLineBitsAndWriteAddressOffset(countBytes, bpp);
		countBytes = currentBit >> 3;
	}

	packedData = (ubyte*)malloc(countBytes);
	memcpy(packedData, tempBuff, countBytes);

	packPercentage = (countBytes * 100) / ((width * height * bpp) >> 3);

	return packedData;
}
