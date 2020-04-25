/*	@(#)ftok.c	1.2	*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

key_t
ftok(path, id)
char *path;
char id;
{
	struct stat st;

	return(stat(path, &st) < 0 ? (key_t)-1 :
	       (key_t)((key_t)id << 24 |
	       (minor(st.st_dev) << 8) ^ (st.st_ino & 0xffffff) ));
}
