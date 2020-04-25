/*	@(#)getlogin.c	1.4	*/
/*	@(#)getlogin.c	1.2	*/
/*LINTLIBRARY*/
#include <sys/types.h>
#include "utmp.h"

#define NULL 0

extern long lseek();
extern int open(), read(), close(), ttyslot();

static struct utmp ubuf;

char *
getlogin()
{
	register me, uf;
	register char *cp;

	if((me = ttyslot()) < 0)
		return(NULL);
	if((uf = open(UTMP_FILE, 0)) < 0)
		return(NULL);
	(void) lseek(uf, (long)(me * sizeof(ubuf)), 0);
	if(read(uf, (char*)&ubuf, sizeof(ubuf)) != sizeof(ubuf)) {
		(void) close(uf);
		return(NULL);
	}
	(void) close(uf);
	ubuf.ut_name[sizeof (ubuf.ut_name)] = ' ';
	for (cp = ubuf.ut_name; *cp++ != ' '; )
		;
	*--cp = '\0';
	return(ubuf.ut_name);
}
