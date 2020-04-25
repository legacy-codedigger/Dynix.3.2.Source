/*
	closedir -- C library extension routine

	last edit:	21-Jan-1984	D A Gwyn
*/

#include	<dir.h>

extern void	free();
extern int	close();

void
closedir( dirp )
	register DIR	*dirp;		/* stream from opendir() */
	{
	(void)close( dirp->dd_fd );
	free( (char *)dirp );
	}
