/*
	opendir -- C library extension routine

	last edit:	19-Dec-1984	D A Gwyn
*/

#include	<dir.h>
#include	<sys/types.h>
#include	<sys/stat.h>

extern char	*malloc();
extern int	open(), close(), fstat();

#define NULL	0

DIR *
opendir( filename )
	char		*filename;	/* name of directory */
	{
	register DIR	*dirp;		/* -> malloc'ed storage */
	register int	fd;		/* file descriptor for read */
	struct stat	sbuf;		/* result of fstat() */

	if ( (fd = open( filename, 0 )) < 0 )
		return NULL;

	if ( fstat( fd, &sbuf ) < 0
	  || (sbuf.st_mode & S_IFMT) != S_IFDIR
	  || (dirp = (DIR *)malloc( sizeof(DIR) )) == NULL
	   )	{
		(void)close( fd );
		return NULL;		/* bad luck today */
		}

	dirp->dd_fd = fd;
	dirp->dd_loc = dirp->dd_size = 0;	/* refill needed */

	return dirp;
	}
