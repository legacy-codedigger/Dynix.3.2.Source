/*
	time -- system call emulation for 4.2BSD

	last edit:	01-Jul-1983	D A Gwyn
*/

extern int	_gettimeofday();

#define NULL	0

long
time( tloc )
	long	*tloc;			/* where to put time */
	{
	struct	{
		unsigned long	tv_sec;
		long		tv_usec;
		}	timeval;	/* receives time */
	struct	{
		int		tz_minuteswest;
		int		tz_dsttime;
		}	timezone;	/* receives time zone */

	if ( _gettimeofday( &timeval, &timezone ) != 0 )
		return -1;		/* "can't happen" */

	if ( tloc != NULL )
		*tloc = timeval.tv_sec;

	return timeval.tv_sec;
	}
