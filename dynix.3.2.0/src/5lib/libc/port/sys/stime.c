/*
	stime -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

extern int	_gettimeofday(), _settimeofday();

int
stime( tp )
	long	*tp;			/* -> time to be set */
	{
	struct	{
		unsigned long	tv_sec;
		long		tv_usec;
		}	timeval;	/* for _settimeofday() */
	struct	{
		int		tz_minuteswest;
		int		tz_dsttime;
		}	timezone;	/* set up by _gettimeofday() */

	if ( _gettimeofday( &timeval, &timezone ) != 0 )
		return -1;		/* "can't happen" */

	timeval.tv_sec = *tp;
	timeval.tv_usec = 0L;

	return _settimeofday( &timeval, &timezone );
	}
