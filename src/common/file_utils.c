#include "file_utils.h"

#include "filestream.h"
#include "filestreamfunctions.h"


Stream *openFileStream(char *path)
{
	Stream *CDstream = OpenDiskStream(path, 0);

	return CDstream;
}

void moveFileStreamPointer(int offset, Stream *CDstream)
{
	if (CDstream) {
		SeekDiskStream(CDstream, offset, SEEK_SET);
	}
}

void moveFileStreamPointerRelative(int offset, Stream *CDstream)
{
	if (CDstream) {
		SeekDiskStream(CDstream, offset, SEEK_CUR);
	}
}

void readSequentialBytesFromFileStream(int size, void *dst, Stream *CDstream)
{
	if (CDstream) {
		ReadDiskStream(CDstream, dst, size);
	}
}

void readBytesFromFileStream(int offset, int size, void *dst, Stream *CDstream)
{
	if (CDstream) {
		SeekDiskStream(CDstream, offset, SEEK_SET);
		ReadDiskStream(CDstream, dst, size);
	}
}

void closeFileStream(Stream *CDstream)
{
	if (CDstream) {
		CloseDiskStream(CDstream);
	}
}

void readBytesFromFileAndClose(char *path, int offset, int size, void *dst)
{
	Stream *CDstream = OpenDiskStream(path, 0);

	if (CDstream) {
		SeekDiskStream(CDstream, offset, SEEK_SET);
		ReadDiskStream(CDstream, dst, size);
		CloseDiskStream(CDstream);
	}
}
