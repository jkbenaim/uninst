#pragma once
#include <stdio.h>
#include <inttypes.h>

/* Flags for files listed in the idb file.
 * If you change this list, then also adjust the size
 * of 'flags' in struct idbline_s.
 */
enum flags_e {
	FLAG_NEEDRQS = 1,
	FLAG_NOHIST = 2,
	FLAG_NOSHARE = 4,
	FLAG_NOSTRIP = 8,
	FLAG_STRIPDSO = 16,
	FLAG_DELHIST = 32,
	FLAG_NORQS = 64,
	FLAG_SHADOW = 128
};

/* The idbline_s struct represents all the information
 * present in one line of the idb file.
 */
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

enum {
	IDBLEX_CONTINUE = 0,
	IDBLEX_STOP = 1
};

/* Callback function type for the idblex function.
 * The callback should return IDBLEX_CONTINUE to continue parsing,
 * or IDBLEX_STOP to quit parsing immediately.
 */
typedef int (*idblex_cb_func_t)(struct idbline_s *line, void *data);

/* The idblex function parses an idb file, making calls to a
 * caller-specified callback function for each line.
 * QUIRKY API WARNING: idblex() needs TWO EXTRA NULLS at the end of the buffer, which will not be parsed.
 */
extern void idblex(char *buf, size_t len, idblex_cb_func_t func, void *data);

