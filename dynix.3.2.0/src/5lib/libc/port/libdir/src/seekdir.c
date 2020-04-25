/*
	seekdir -- C library extension routine

	last edit:	21-Jan-1984	D A Gwyn
*/

#include	<dir.h>

extern long	lseek();

#define NULL	0

void
seekdir( dirp, loc )
	register DIR	*dirp;		/* stream from opendir() */
	long		loc;		/* position from telldir() */
	{
	long		base;		/* file location of block */
	long		offset; 	/* offset within block */

	if ( telldir( dirp ) == loc )
		return; 		/* save time */

	offset = loc % DIRBLKSIZ;
	base = loc - offset;

	(void)lseek( dirp->dd_fd, base, 0 );	/* change blocks */
	dirp->dd_loc = dirp->dd_size = 0;

	while ( dirp->dd_loc < offset ) /* skip entries */
		if ( readdir( dirp ) == NULL )
			return; 	/* "can't happen" */
	}
