#pragma once

extern char *read_string_from_image(int infd);
extern int copypipe(int infd, int outfd, unsigned inlen);
