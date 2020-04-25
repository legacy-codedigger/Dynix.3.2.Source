/*
	open -- system call emulation for 4.2BSD

	last edit:	15-Dec-1983	D A Gwyn
*/

#include	<errno.h>
#include	<fcntl.h>

#define FIONBIO 0x8004667e		/* 4.2BSD _ioctl() request */

extern int	_open(), access(), _ioctl();
extern int	errno;

int
/*VARARGS2*/
open( path, oflag, mode )		/* returns fildes, or -1 */
	register char	*path;		/* pathname of file */
	register int	oflag;		/* flag bits, see open(2) */
	int		mode;		/* O_CREAT protection mode */
	{
	register int	fd;		/* file descriptor */
	int		serrno = errno; /* save original errno */

	fd = _open( path, oflag, mode );

	/* 4.2BSD emulation of O_NDELAY requires non-blocking I/O: */

	if ((fd != -1) && ((oflag & O_NDELAY) != 0))
		{
		static int	on = 1; /* stupid syscall design */

		(void)_ioctl( fd, FIONBIO, (char *)&on );
		errno = serrno; 	/* restore errno */
		}

	return fd;
	}
