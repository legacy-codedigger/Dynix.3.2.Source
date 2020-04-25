/*
	ptrace -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

#include	<errno.h>
#include	<signal.h>

extern int	_ptrace();
extern int	errno;

int
ptrace( request, pid, addr, data )
	register int	request;	/* request code */
	int		pid;		/* child process ID */
	int		addr;		/* address of data */
	int		data;		/* data to be stored */
	{
	register int	result; 	/* _ptrace() value */
	register int	sdata;		/* save data for request 7 */

	switch ( request )
		{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 8:
		break;

	case 7:
	case 9:
		sdata = data;		/* save for return value */
		if ( sdata >= 0 && sdata <= NSIG )
			break;
		/* bad signal #; fall through into error return */

	default:			/* bad request # */
		errno = EIO;
		return -1;
		}

	if ((result = _ptrace( request, pid, (int *)addr, data )) < 0 )
		switch ( errno )
			{
		case EINVAL:		/* PID doesn't exist */
			errno = ESRCH;
			return -1;

		case EFAULT:		/* address out of bounds */
			errno = EIO;
			/* fall through into error return */

		default:		/* errno already reasonable */
			return -1;
			}
	else if ( request == 7 || request == 9 )
		return sdata;
	else
		return result;
	}
