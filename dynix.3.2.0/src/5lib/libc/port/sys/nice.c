/*
	nice -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

#include	<sgtty.h>
#include	<errno.h>

extern int	_getpriority(), _setpriority(), getpid(), getuid(), geteuid();

int
nice( incr )
	int		incr;
	{
	register int	serrno = errno; /* entry errno, probably 0 */
	register int	pid = getpid(); /* process ID */
	register int	prio = _getpriority( 0, pid );	/* priority */
	register int	notsuser = getuid() && geteuid(); /* super user */

	if ( prio == -1 && errno != serrno )
		return -1;		/* _getpriority() error */

	prio += incr;
	if( notsuser ) {
		if ( (prio < -20) || (prio > 19) ) {
			errno = EPERM;
			return -1;
		}
	} else {
		if ( prio < -20 )
			prio = -20;
		else if ( prio > 19 )
			prio = 19;
	}

	serrno = _setpriority( 0, pid, prio );
	if( serrno == 0 )
		return prio;
	errno = EPERM;
	return -1;
}
