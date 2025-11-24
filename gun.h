#pragma once
enum zrc_e {
	Z_BUF_ERROR = -5,
	Z_DATA_ERROR = -3,
	Z_OK = 0
};
extern enum zrc_e gunpipe(int infile, int outfile, ssize_t bytes_left);
