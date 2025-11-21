#pragma once
#include <stdio.h>
#include <inttypes.h>

enum flags_e {
	FLAG_NEEDRQS = 1,
	FLAG_NOHIST = 2,
	FLAG_NOSHARE = 4,
	FLAG_NOSTRIP = 8,
	FLAG_STRIPDSO = 16,
	FLAG_DELHIST = 32,
	FLAG_NORQS = 64,
	FLAG_SHADOW = 128
	// If you change this list, then also adjust the size of 'flags' in struct idbline_s.
};

struct idbline_s {
	int line_num;
	char type;
	uint8_t flags;
	uint16_t mode;
	char *userName;
	char *groupName;
	char *installPath;
	char *sourcePath;
	char *subsystem;
	char *config1;
	char *exitop;
	char *mach;
	char *postop;
	char *preop;
	char *removeop;
	char *symval;
	char *mac;
	uint32_t cmpsize;
	uint32_t off;
	uint32_t size;
	uint32_t sum;
	uint32_t f;
	bool cmpsize_present, off_present, size_present, sum_present, f_present;
};

// QUIRKY API WARNING: idblex() needs TWO EXTRA NULLS at the end of
// the buffer, which will not be parsed.
extern void idblex(int64_t file_id, char *buf, size_t len);
