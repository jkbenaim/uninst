#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cksum.h"
#include "copypipe.h"
#include "endian.h"
#include "err.h"

char *read_string_from_image(int infd)
{
	ssize_t sRc;
	uint16_t len;
	char buf[2];
	char *name = NULL;

	/* The first two bytes are a length, followed by a string
	 * of that length indicating the install path. The
	 * string is not null-terminated. We don't do anything
	 * with this string except seek past it.
	 */
	sRc = read(infd, buf, 2);
	if (sRc != 2) err(1, "couldn't read from image");
	memcpy(&len, buf, 2);
	len = be16toh(len);

	name = malloc(len + 1);
	if (!name) err(1, "in malloc");
	sRc = read(infd, name, len);
	if (sRc != len) err(1, "couldn't read from image 2");
	name[len] = '\0';

	/* At this point, the infile is positioned right at
	 * the beginning of the file data.
	 */

	return name;
}

int copypipe(int infd, int outfd, unsigned inlen)
{
	ssize_t sRc;
	unsigned char buf[BUFSIZ];
	uint16_t sum = 0;

	while (inlen > 0) {
		ssize_t step;

		step = inlen>BUFSIZ?BUFSIZ:inlen;
		sRc = read(infd, buf, step);
		if (sRc < 0) err(1, "couldn't read from image");

		if (sRc == 0) continue;
		sum = cksum_update(buf, step, sum);
		step = sRc;
		if (outfd != -1) {
			sRc = write(outfd, buf, sRc);
			if (sRc == -1) err(1, "couldn't write to outfile");
		}
		inlen -= step;
	}

	return sum;
}
