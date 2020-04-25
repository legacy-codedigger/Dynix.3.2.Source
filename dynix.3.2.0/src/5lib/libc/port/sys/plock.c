/*
	plock -- system call emulation for 4.2 BSD
*/

#include	<errno.h>
#include	<sys/lock.h>


static	int u_lock;		/* keep lock bits around */
static  int u_lock_pid;		/* must clear after a fork! */

int
plock( op )
	int	op;		/* operation */
{

	if (getuid() != 0 && geteuid() != 0) {
		errno = EPERM;
		return -1;
	}
	if (u_lock_pid != getpid()) {	/* clear lock bits after a fork */
		u_lock = 0;
		u_lock_pid = getpid();
	}
	switch ( op ) {
	case TXTLOCK:
		if ((u_lock&(PROCLOCK|TXTLOCK)) || textlock() == 0)
			goto bad;
		break;

	case PROCLOCK:
		if ((u_lock&(PROCLOCK|TXTLOCK|DATLOCK))  ||
		    datalock() == 0  ||
		    proclock() == 0  )
			goto bad;
		(void) textlock();
		break;

	case DATLOCK:
		if ((u_lock&(PROCLOCK|DATLOCK)) || datalock() == 0)
			goto bad;
		break;

	case UNLOCK:
		if (punlock() == 0)
			goto bad;
		break;

	default:
bad:
		errno = EINVAL;
		return -1;
	}
	return 0;
}

static int
textlock()
{
	u_lock |= TXTLOCK;
	return 1;
}

static int
tunlock()
{
	u_lock &= ~TXTLOCK;
	return 1;
}

static int
datalock()
{
	u_lock |= DATLOCK;
	return 1;
}

static int
dunlock()
{
	u_lock &= ~DATLOCK;
	return 1;
}

static int
proclock()
{
	u_lock |= PROCLOCK;
	return 1;
}

static int
punlock()
{
	if ((u_lock&(PROCLOCK|TXTLOCK|DATLOCK)) == 0)
		return 0;
	u_lock &= ~PROCLOCK;
	(void) tunlock();
	(void) dunlock();
	return 1;
}
