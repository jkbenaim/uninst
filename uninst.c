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
#include "copy.h"
#include "endian.h"
#include "err.h"
#include "gun.h"
#include "idblex.h"
#include "mapfile.h"
#include "progname.h"
#include "stdnoreturn.h"
#include "tryhelp.h"
#include "version.h"

noreturn static void usage(void);

int Lflag = 0;
int Vflag = 0;

static char *openfilename = NULL;
static int fd = -1;

/* Drop-in for open() that also creates directories along the way.
 * 
 * NOTE: open is defined as:
 * 	int open(const char *file, int flag, ...);
 * with an optional 3rd argument, specifying the file mode
 * if O_CREAT or O_TMPFILE is set in flags.
 * In our open_mkdir, we assert that O_CREAT was passed
 * in flags, and so the mode paramater is mandatory too.
 */
int open_mkdir(const char *file, int flags, int mode)
{
	int rc;
	char *myclone, *tmp;
	assert(flags & O_CREAT);
	myclone = strdup(file);
	if (!myclone) err(1, "in strdupa");

	/* Walk the path, creating directories as we go. */
	for (tmp = myclone; *tmp; tmp++) {
		if (*tmp == '/') {
			*tmp = '\0';
			/* WEIRD API ALERT:
			 * POSIX and Win32 have different definitions of mkdir().
			 * Fortunately, the only real difference is in whether they
			 * take a second argument. Windows doesn't, but POSIX takes
			 * an argument specifying the new directory's mode.
			 * On error, either implementation will return -1
			 * and set errno appropriately.
			 */
#ifdef __MINGW32__
			rc = mkdir(myclone);
#else
			rc = mkdir(myclone, 0755);
#endif
			if ((rc == -1) && (errno != EEXIST)) {
				free(myclone);
				return rc;
			}
			*tmp = '/';
		}
	}
	free(myclone);
	return open(file, flags, mode);
}

int main(int argc, char *argv[])
{
	char *filename = NULL;
	int rc;
	size_t sz;

	progname_init(argc, argv);
	
	opterr = 0;
	while ((rc = getopt(argc, argv, ":hlvV")) != -1)
		switch (rc) {
		case 'h':
			usage();
			break;
		case 'l':
			if (Lflag)
				tryhelp("option '-%c' can only be used once", rc);
			Lflag = 1;
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

	char *pdfilename;
	/* Before we go searching, let's see what kind of file the user
	 * specified. */
	char *dot;
	char *temp = strdup(filename);
	if (!temp) err(1, "in strdup");
	dot = strrchr(temp, '.');
	if (dot) {
		dot = '\0';
		if (NULL != strchr(temp, '.'))
			errx(1, "given file has too many dots in its name");
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

	int callback(struct idbline_s *line, void *data)
	{
		__label__ next_file;
		int rc;
		int outfd = -1;
		struct stat sb;
		char *imagename = NULL;
		char *pdfilename = (char *)data;
		if (Lflag || Vflag)
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
		
		/* If an image file is open, then it's definitely the one want.
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
		}

		assert(openfilename != NULL);
		assert(fd != -1);

		if (!line->off_present || !line->size_present)
			goto next_file;

		int flags = O_WRONLY | O_CREAT;
#ifdef __MINGW32__
		flags |= O_BINARY;
#endif
		outfd = open_mkdir(line->installPath, flags, 0644);
		if (outfd == -1) err(1, "couldn't open outfile '%s'", line->installPath);

		off_t pos;
		pos = lseek(fd, line->off, SEEK_SET);
		if (pos == (off_t)(-1))
			err(1, "while seeking image");
		seek_past_name(fd);
		if (line->cmpsize_present && (line->cmpsize > 0)) {
			/* Data is compressed. */
			rc = gunpipe(fd, outfd, line->cmpsize);
		} else {
			/* Data is raw. */
			copy(fd, outfd, line->size);
		}
next_file:
		if (outfd > 0) {
			close(outfd);
			outfd = -1;
		}
		free(temp);
		free(imagefilename);
		return IDBLEX_CONTINUE;
	}
	idblex(idbdata, sb.st_size + 2, callback, (void *)pdfilename);

	free(pdfilename);
	pdfilename = NULL;
	free(idbdata);
	idbdata = NULL;
	free(openfilename);
	close(fd);

	return EXIT_SUCCESS;
}

noreturn static void usage(void)
{
	(void)fprintf(stderr,
"Usage: %s [OPTION] <FILE>\n"
"Extract files from the IRIX 'inst' package in FILE.\n"
"\n"
"  -h       print this help text\n"
"  -l       list files instead of extracting\n"
"  -v       list files while extracting\n"
"  -V       print program version\n"
"\n"
"Please report any bugs to <%s>.\n"
,		__progname,
		PROG_EMAIL
	);
	exit(EXIT_FAILURE);
}
