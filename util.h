#pragma once

struct path_s {
	char *dirname;
	char *basename;
	char *_freeme[2];
};

extern struct path_s path_decompose(const char *filename);
extern struct path_s path_free(struct path_s p);
extern int open_mkdir(const char *file, int flags, int mode);
