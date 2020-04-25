/*
	ulimit -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

#include	<errno.h>

extern int	_getrlimit(), _setrlimit();
extern int	errno;

long
ulimit( cmd, newlimit )
	int	cmd;			/* subcommand */
	long	newlimit;		/* desired new limit */
	{
	struct	{
		long	rlim_cur;
		long	rlim_max;
		}	limit;		/* data being gotten/set */

	switch ( cmd )
		{
	case 1: 			/* get file size limit */
		if ( _getrlimit( 1, &limit ) != 0 )
			return -1L;	/* errno is already set */
		return limit.rlim_max / 512L;

	case 2: 			/* set file size limit */
		if ((unsigned)(newlimit * 512L) < 0 ) {
			errno = EINVAL;
			return -1l;
		}
		if ( _getrlimit( 1, &limit ) != 0 )
			return -1L;	/* errno is already set */
		/* Only root can raise your current file size under ATT unix */
		if ( newlimit * 512L > (unsigned) limit.rlim_cur && getuid() && geteuid() ) {
			errno = EPERM;
			return -1L;
		}
		limit.rlim_cur = limit.rlim_max = newlimit * 512L;
		_setrlimit( 1, &limit );
		return limit.rlim_max / 512L;

	case 3: 			/* get maximum break value */
		if ( _getrlimit( 2, &limit ) != 0 )
			return -1L;	/* errno is already set */
		return limit.rlim_max;

	default:
		errno = EINVAL;
		return -1L;
		}
	}
