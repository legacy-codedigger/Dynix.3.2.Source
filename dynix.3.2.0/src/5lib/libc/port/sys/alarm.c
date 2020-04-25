/*
	alarm -- system call emulation for 4.2BSD

	last edit:	11-Jul-1984	D A Gwyn
*/

extern int	_setitimer();

typedef struct
	{
	unsigned long	tv_sec;		/* seconds */
	long		tv_usec;	/* microseconds */
	}	timeval;

typedef struct
	{
	timeval		it_interval;	/* timer interval */
	timeval		it_value;	/* current value */
	}	itimerval;

unsigned
alarm( sec )
	unsigned	sec;		/* timeout in seconds */
	{
	itimerval	newit;		/* new interval data */
	itimerval	oldit;		/* old interval data */

	/* set alarm clock timeout interval (0 disables) */
	newit.it_value.tv_sec = (unsigned long)sec;
	newit.it_value.tv_usec = 0L;

	/* avoid retriggering once timer expires */
	newit.it_interval.tv_sec = 0L;
	newit.it_interval.tv_usec = 0L;

	if ( _setitimer( 0, &newit, &oldit ) < 0 )	/* real time */
		return -1;
	/* SIGALRM now pending */

	return (unsigned)oldit.it_value.tv_sec +
			(oldit.it_value.tv_usec > 500000L ? 1 : 0);
	}
