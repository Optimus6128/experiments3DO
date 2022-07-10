#include "file_utils.h"

#include "filestream.h"
#include "filestreamfunctions.h"

#define FILE_BUFF_SIZE 65536

static char fileBuff[FILE_BUFF_SIZE];
Stream *CDstream = NULL;

void openFileStream(char *path)
{
	CDstream = OpenDiskStream(path, 0);
}

void moveFilePointer(int offset)
{
	if (CDstream) {
		SeekDiskStream(CDstream, offset, SEEK_SET);
	}
}

void moveFilePointerRelative(int offset)
{
	if (CDstream) {
		SeekDiskStream(CDstream, offset, SEEK_CUR);
	}
}

char *readSequentialBytesFromFile(int size)
{
	if (!CDstream) return NULL;

	ReadDiskStream(CDstream, fileBuff, size);

	return fileBuff;
}

char *readBytesFromFile(int offset, int size)
{
	if (!CDstream) return NULL;

	SeekDiskStream(CDstream, offset, SEEK_SET);
	ReadDiskStream(CDstream, fileBuff, size);

	return fileBuff;
}

void closeFileStream()
{
	if (CDstream) CloseDiskStream(CDstream);
}

char *readBytesFromFileAndClose(char *path, int offset, int size)
{
	uint8 *fileBuffPtr = NULL;

	CDstream = OpenDiskStream(path, 0);

	fileBuffPtr = readBytesFromFile(offset, size);
	
	if (CDstream) CloseDiskStream(CDstream);

	return fileBuffPtr;
}
