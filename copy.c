#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "copy.h"
#include "endian.h"
#include "err.h"

void seek_past_name(int infd)
{
	ssize_t sRc;
	uint16_t len;
	unsigned char buf[BUFSIZ];
	
	/* The first two bytes are a length, followed by a string
	 * of that length indicating the install path. The
	 * string is not null-terminated. We don't do anything
	 * with this string except seek past it.
	 */
	sRc = read(infd, buf, 2);
	if (sRc != 2) err(1, "couldn't read from image");
	memcpy(&len, buf, 2);
	len = be16toh(len);
	assert(len <= BUFSIZ);
	sRc = read(infd, buf, len);
	if (sRc != len) err(1, "couldn't read from image 2");
	/* At this point, the infile is positioned right at
	 * the beginning of the file data.
	 */
}

void copy(int infd, int outfd, unsigned inlen)
{
	ssize_t sRc;
	unsigned char buf[BUFSIZ];
	size_t min(size_t a, size_t b) { return a>b?b:a; }

	while (inlen > 0) {
		ssize_t step;

		sRc = read(infd, buf, min(inlen, BUFSIZ));
		if (sRc < 0) err(1, "couldn't read from image");

		if (sRc == 0) continue;
		step = sRc;
		sRc = write(outfd, buf, sRc);
		if (sRc == -1) err(1, "couldn't write to outfile");
		inlen -= step;
	}
}
