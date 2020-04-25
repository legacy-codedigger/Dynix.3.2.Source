/* @(#)uparm.h	1.4 */
/*
 * Local configuration of various files.  Used if you can't put these
 * things in the standard places or aren't the super user, so you
 * don't have to modify the source files.  Thus, you can install updates
 * without having to re-localize your sources.
 */

/* Path to library files */
#define libpath(file) "/usr/lib/file"

/* Path to local library files */
#define loclibpath(file) "/usr/local/lib/file"

/* Path to binaries */
#define binpath(file) "/usr/bin/file"

/* Path to things under /usr (e.g. /usr/preserve) */
#define usrpath(file) "/usr/file"

/* Location of terminfo binary directory tree */
#define termpath(file)	"/usr/lib/terminfo/file"

#define TMPDIR	"/tmp"
