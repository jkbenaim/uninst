#pragma once
#include <stddef.h>
enum zrc_e {
	Z_BUF_ERROR = -5,
	Z_DATA_ERROR = -3,
	Z_OK = 0
};

/* unlzwpipe returns a negative number on error, or the 16-bit
 * checksum of the decompressed data.
 */
extern int unlzwpipe(int infile, int outfile, ssize_t bytes_left);
