#include <assert.h>
#include <errno.h>
#include <fcntl.h>	/* for O_CREAT */
#include <sys/stat.h>	/* for mkdir(2) */
#include <stdlib.h>
#include <string.h>
#include "err.h"
#include "util.h"

#if defined(__MINGW32__)
#define DIRSEP '\\'
#else
#define DIRSEP '/'
#endif

struct path_s path_decompose(const char *filename)
{
	struct path_s out = {NULL, NULL, {NULL, NULL}};
	char *tmp;
	char *myfilename = strdup(filename);
	if (!myfilename) err(1, "in strdup");

	tmp = strrchr(myfilename, DIRSEP);
	if (!tmp) {
		out.dirname = strdup("");
		if (!out.dirname) err(1, "in strdup");
		out.basename = myfilename;
		out._freeme[0] = out.dirname;
		out._freeme[1] = out.basename;
		return out;
	}

	tmp[0] = '\0';
	out.dirname = myfilename;
	out.basename = &tmp[1];
	out._freeme[0] = out.dirname;
	return out;
}

struct path_s path_free(struct path_s p)
{
	free(p._freeme[0]);
	free(p._freeme[1]);
	return (struct path_s){NULL, NULL, {NULL, NULL}};
}

/* Drop-in for open() that also creates directories along the way.
 * 
 * NOTE: open is defined as...
 * 	int open(const char *file, int flag, ...);
 * ... with an optional 3rd argument, specifying the file mode if
 * either O_CREAT or O_TMPFILE is set in flags.
 * In our open_mkdir, we assert that O_CREAT was passed in flags,
 * and so the mode paramater is mandatory too.
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
