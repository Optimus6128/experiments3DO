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

void readBytesFromFileAndStore(char *path, int offset, int size, void *dst)
{
	CDstream = OpenDiskStream(path, 0);

	SeekDiskStream(CDstream, offset, SEEK_SET);
	ReadDiskStream(CDstream, dst, size);
	
	if (CDstream) CloseDiskStream(CDstream);
}
