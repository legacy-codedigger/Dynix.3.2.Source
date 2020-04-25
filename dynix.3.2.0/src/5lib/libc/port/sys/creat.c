/*
	creat -- old system call emulation using UNIX System V open()

	last edit:	06-Jul-1983	D A Gwyn
*/

#include	<fcntl.h>

extern int	_open();

int
creat( path, mode )
	char	*path;			/* name of new file */
	int	mode;
	{
	/* avoid open() emulation overhead: */
	return _open( path, O_WRONLY | O_CREAT | O_TRUNC, mode );
	}
