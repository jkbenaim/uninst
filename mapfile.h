#ifndef _MAPFILE_H_
#define _MAPFILE_H_

#ifdef __MINGW32__
#include <windows.h>
#endif
#include <stdbool.h>
#include <stdint.h>

struct MappedFile_s {
	void *data;
	uint64_t size;
#ifdef __MINGW32__
	HANDLE _hFile;
	HANDLE _hMapping;
#else
	int _fd;
#endif
};

extern struct MappedFile_s MappedFile_Create(const char *filename, size_t size);
extern struct MappedFile_s MappedFile_Open(const char *filename, bool writable);
extern struct MappedFile_s MappedFile_Close(struct MappedFile_s m);

/* _MAPFILE_H_ */
#endif
