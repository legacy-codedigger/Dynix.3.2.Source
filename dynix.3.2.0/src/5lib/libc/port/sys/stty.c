/*
	stty -- system call emulation for 4.2BSD and BRL PDP-11 UNIX

	last edit:	28-Aug-1983	D A Gwyn
*/

#include	<sgtty.h>

extern int	ioctl();

int
stty( fildes, args )
	int		fildes;
	struct sgttyb	*args;
	{
	return ioctl( fildes, TIOCSETP, (int)args );
	}
