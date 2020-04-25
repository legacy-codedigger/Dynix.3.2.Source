/*
	setgid -- system call emulation for 4.2BSD

	last edit:	02-Jul-1983	D A Gwyn
*/

#include	"errno.h"

extern int	_setregid();
extern int	errno;

int
setgid( gid )
	int	gid;
{
	if (gid < 0) {
		errno = EINVAL;
		return (-1);
	}
	return (_setregid( gid, gid ));
}
