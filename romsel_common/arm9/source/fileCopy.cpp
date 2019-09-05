#include <nds.h>
#include <stdio.h>
#include "fileCopy.h"

#define COPY_BUF_SIZE 131072

off_t getFileSize(const char *fileName)
{
	FILE* fp = fopen(fileName, "rb");
	off_t fsize = 0;
	if (fp) {
		fseeko(fp, 0, SEEK_END);
		fsize = ftello(fp);	// Get source file's size
		fseeko(fp, 0, SEEK_SET);
	}
	fclose(fp);

	return fsize;
}

int fcopy(const char *sourcePath, const char *destinationPath)
{
	FILE* sourceFile = fopen(sourcePath, "rb");
	if (!sourceFile) {
		return 1;
	}

	// Get source file's size
	fseeko(sourceFile, 0, SEEK_END);
	off_t fsize = ftello(sourceFile);
	fseeko(sourceFile, 0, SEEK_SET);

	u8 *buf = (u8*)malloc(COPY_BUF_SIZE);
	if (!buf) {
		fclose(sourceFile);
		return 1;
	}

	FILE* destinationFile = fopen(destinationPath, "wb");
	if (!destinationFile) {
		free(buf);
		fclose(sourceFile);
		return 1;
	}

	off_t offset = 0;
	while (1)
	{
		/* scanKeys();
		if (keysHeld() & KEY_A) {
			// Cancel copying
			fclose(sourceFile);
			fclose(destinationFile);
			return -1;
			break;
		} */

		// Copy file to destination path
		size_t numr = fread(buf, 1, COPY_BUF_SIZE, sourceFile);
		fwrite(buf, 1, numr, destinationFile);
		offset += COPY_BUF_SIZE;

		if (offset >= fsize) {
			free(buf);
			fclose(destinationFile);
			fclose(sourceFile);
			return 0;
		}
	}

	// Should not get here...
	return 1;
}
