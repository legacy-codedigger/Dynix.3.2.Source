/*
	kill -- system call emulation for 4.2BSD

	last edit:	02-Jan-1985	D A Gwyn
*/

#include	<errno.h>
#include	<sys/signal.h>

extern int	_kill(), _killpg(), getpid();

int
kill( pid, sig )
	register int	pid;		/* process ID or special code */
	register int	sig;		/* signal to be sent */
	{
	register int	retval;		/* function return value */

	if ( sig < 0 /* || sig > NSIG */
	  || sig == SIGKILL && pid == 1	/* undocumented kernel check */
	   )	{
		errno = EINVAL;		/* kernel would've allowed it */
		return -1;
		}

	if ( pid < -1 )
		return _killpg( -pid, sig );
	else	if ( (retval = _kill( pid, sig )) == 0 && pid == -1 )
			return _kill( getpid(), sig );
		else
			return retval;

	/* The case PID == -1 and UID != 0 is not handled right, since
	   it would require testing the UID of all running processes. */
	}
