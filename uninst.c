#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cksum.h"
#include "copypipe.h"
#include "endian.h"
#include "err.h"
#include "idblex.h"
#include "progname.h"
#include "stdnoreturn.h"
#include "tryhelp.h"
#include "unlzw.h"
#include "util.h"
#include "version.h"

noreturn static void usage(void);

int exit_val = EXIT_SUCCESS;
int Lflag = 0;
int Vflag = 0;
int Tflag = 0;

static char *openfilename = NULL;
static int fd = -1;

int callback(struct idbline_s *line, void *data)
{
	__label__ next_file, out_error;
	int rc;
	int outfd = -1;
	struct stat sb;
	char *imagename = NULL;
	char *pdfilename = (char *)data;
	if ((Lflag || Vflag) && !Tflag)
		printf("%s\n", line->installPath);
	if (Lflag) return 0;
	
	/* Find the image filename, using the subsystem name as a base.
	 * Subsystem names are listed in the file as:
	 * 	x.y.z
	 * where 'z' is just the plain subsystem name,
	 * 'y' is the image name,
	 * and 'x' is the product name.
	 * What we want is just 'y'.
	 */

	char *temp;
	temp = imagename = strdup(line->subsystem);
	if (!imagename) err(1, "in strdup");

	char *dot = strrchr(imagename, '.');
	if (!dot) errx(1, "no last dot in subsystem name '%s'", imagename);
	*dot = '\0';
	
	dot = strchr(imagename, '.');
	if (!dot) errx(1, "no first dot in image name '%s'", imagename);
	*dot = '\0';
	imagename = &dot[1];

	/* Assert that imagename has no dots in it. */
	if (strchr(imagename, '.')) errx(1, "image name has dots");

	char *imagefilename = NULL;
	rc = asprintf(&imagefilename, "%s.%s", pdfilename, imagename);
	if (rc == -1) err(1, "in asprintf");

	/* If an image file is already open, and it's not the one we
	 * want, then close that image file.
	 */
	if (openfilename && (0 != strcmp(openfilename, imagefilename))) {
		if (fd > 0) rc = close(fd);
		free(openfilename);
		openfilename = NULL;
		fd = -1;
	}
	
	/* If an image file is open, then it's definitely the one we want.
	 * If there is no open image file, then we open the one we want.
	 */
	if (!openfilename) {
		int flags = O_RDONLY;
#ifdef __MINGW32__
		flags |= O_BINARY;
#endif
		openfilename = strdup(imagefilename);
		rc = stat(openfilename, &sb);
		if (rc) err(1, "couldn't stat image file '%s'", openfilename);

		fd = open(openfilename, flags);
		if (fd == -1) err(1, "while opening image file '%s'", openfilename);

		/* Seek past the image magic. */
		off_t pos;
		pos = lseek(fd, 13, SEEK_SET);
		if (pos == (off_t)(-1))
			err(1, "while seeking past image magic");
	}

	assert(openfilename != NULL);
	assert(fd != -1);

	if (!line->size_present) {
		if (line->type == 'f')
			warnx("skipping file '%s' as no size present", line->installPath);
		goto next_file;
	}

	int flags = O_WRONLY | O_CREAT;
#ifdef __MINGW32__
	flags |= O_BINARY;
#endif
	if (Tflag) {
		/* For test mode, don't open any output files.
		 * Both copypipe and unlzwpipe understand that an outfd
		 * of -1 means to simply compute the checksum and return.
		 */
		outfd = -1;
	} else {
		outfd = open_mkdir(line->installPath, flags, 0644);
		if (outfd == -1)
			err(1, "couldn't open outfile '%s'", line->installPath);
	}

	if (line->off_present) {
		off_t pos;
		pos = lseek(fd, line->off, SEEK_SET);
		if (pos == (off_t)(-1))
			err(1, "while seeking image");
	}
	
	/* Check that the install path in the .idb line matches the
	 * install path in the image file.
	 */
	char *checkname;
	checkname = read_string_from_image(fd);
	if (!checkname) errx(1, "couldn't read install path from image file");
	if (0 != strcmp(checkname, line->installPath)) {
		errx(1, "idb install path '%s' doesn't match image install path '%s'",
			line->installPath, checkname);
		free(checkname);
		goto out_error;
	}
	free(checkname);
	checkname = NULL;

	if (line->cmpsize_present && (line->cmpsize > 0)) {
		/* Data is compressed. */
		rc = unlzwpipe(fd, outfd, line->cmpsize);
	} else {
		/* Data is not compressed. */
		rc = copypipe(fd, outfd, line->size);
	}
	if (rc < 0) {
		/* An error occurred. */
		errx(1, "while extracting: %d\n", rc);
	}

	if (line->sum_present) {
		/* Verify checksum. */
		if (line->sum == rc) {
			if (Vflag) {
				fprintf(stderr, "%s:  OK\n", line->installPath);
			}
		} else {
			fprintf(stderr, "%s:  checksum failed\n", line->installPath);
			exit_val = EXIT_FAILURE;
			goto out_error;
		}
	}

next_file:
	if (outfd > 0) {
		close(outfd);
		outfd = -1;
	}
	free(temp);
	free(imagefilename);
	return IDBLEX_CONTINUE;
out_error:
	if (outfd > 0) {
		close(outfd);
		outfd = -1;
	}
	free(temp);
	free(imagefilename);
	warnx("terminating early");
	return IDBLEX_STOP;
}

int main(int argc, char *argv[])
{
	char *filename = NULL;
	int rc;
	size_t sz;

	progname_init(argc, argv);
	
	opterr = 0;
	while ((rc = getopt(argc, argv, ":hltvV")) != -1)
		switch (rc) {
		case 'h':
			usage();
			break;
		case 'l':
			if (Lflag)
				tryhelp("option '-%c' can only be used once", rc);
			Lflag = 1;
			break;
		case 't':
			if (Tflag)
				tryhelp("option '-%c' can only be used once", rc);
			Tflag = 1;
			break;
		case 'v':
			if (Vflag)
				tryhelp("option '-%c' can only be used once", rc);
			Vflag = 1;
			break;
		case 'V':
			fprintf(stderr, "%s\n", PROG_EMBLEM);
			exit(EXIT_SUCCESS);
			break;
		case '?':
			tryhelp("unrecognized option '-%c'", optopt);
			break;
		case ':':
			tryhelp("missing argument after '-%c'", optopt);
		}
	argc -= optind;
	argv += optind;

	if (Lflag && Tflag)
		tryhelp("cannot use -l and -t together");
	if (Lflag && Vflag)
		tryhelp("cannot use -l and -v together");

	if (*argv != NULL) {
		filename = *argv;
	} else {
		tryhelp("must specify a file");
	}

	/* Check that the user-specified file exists. */
	struct stat sb;
	rc = stat(filename, &sb);
	if (rc) err(1, "couldn't stat '%s'", filename);

	/* Great, the file exists. Now we need to find the rest of the files.
	 * An 'inst' package consists of multiple files:
	 *
	 * The "product description" file describes the package, including
	 * the product/image/subsystem heirarchy, dependencies, installation
	 * rules, and so on. There is exactly one product description file per
	 * package.The name of this file is usually just the product's
	 * short name with no extension, like so:
	 * 	<pkgname>
	 * We don't actually need to parse the product description for our
	 * purpose, so we skip it. (Good thing too, because it's pretty hairy!)
	 *
	 * The "idb" file contains a list of all files installed by the
	 * package. For each file, it lists an installation path, the file's
	 * product and subsystem (important later), and its size and offset in
	 * the image files. The idb file is usually named the same as the
	 * product desciption file with ".idb" suffixed, like so:
	 * 	<pkgname>.idb
	 *
	 * The "image" files contain all the file data. A package usually has
	 * multiple image files, one for each system. For example, if the
	 * package has two images named "man" and "sw", then we would
	 * expect to find image files named like so:
	 * 	<pkgname>.man
	 * 	<pkgname>.sw
	 * File data can be compressed with LZW, or not- check the idb file
	 * to find out.
	 */

	/* Do the equivalent of basename(3) but without calling basename(3).
	 * Unfortunately, POSIX's definition of basename(3) is allowed to
	 * modify its argument for some reason. What a happy surprise. */
	char *slash, *temp;
	slash = strrchr(filename, '/');
	if (slash) {
		temp = strdup(&slash[1]);
	} else {
		temp = strdup(filename);
	}

	char *pdfilename;
	char *dot;
	if (!temp) err(1, "in strdup");
	dot = strrchr(temp, '.');
	if (dot) {
		dot = '\0';
		if (NULL != strchr(temp, '.')) {
			warnx("given file has too many dots in its name.");
			errx(1, "are you sure this is the product description file?");
		}
	}
	pdfilename = temp;

	char *idbfilename;
	rc = asprintf(&idbfilename, "%s.idb", pdfilename);
	rc = stat(idbfilename, &sb);
	if (rc) err(1, "couldn't stat idbfile '%s'", idbfilename);
	char *idbdata;
	idbdata = calloc(1, sb.st_size + 2);	/* Why +2? We need two nulls at the end. */
	if (rc == -1) err(1, "in asprintf");
	FILE *idbf = fopen(idbfilename, "rb");
	if (!idbf) err(1, "couldn't open idb file '%s'", idbfilename);
	sz = fread(idbdata, sb.st_size, 1, idbf);
	if (sz != 1) err(1, "while reading from idb file (%zu)", sz);
	rc = fclose(idbf);
	if (rc) err(1, "in fclose");
	free(idbfilename);
	idbfilename = NULL;
	idbf = NULL;

	idblex(idbdata, sb.st_size + 2, callback, (void *)pdfilename);

	free(pdfilename);
	pdfilename = NULL;
	free(idbdata);
	idbdata = NULL;
	free(openfilename);
	close(fd);

	if (exit_val != EXIT_SUCCESS)
		warnx("errors occurred");

	return exit_val;
}

noreturn static void usage(void)
{
	(void)fprintf(stderr,
"Usage: %s [OPTION] <FILE>\n"
"Extract files from the IRIX 'inst' package in FILE.\n"
"\n"
"  -h       print this help text\n"
"  -l       list files instead of extracting\n"
"  -t       test checksum of files in package\n"
"  -v       list files while extracting\n"
"  -V       print program version\n"
"\n"
"Please report any bugs to <%s>.\n"
,		__progname,
		PROG_EMAIL
	);
	exit(EXIT_FAILURE);
}
