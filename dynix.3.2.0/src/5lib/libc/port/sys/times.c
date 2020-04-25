/*
	times -- system call emulation for 4.2BSD
*/

#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/param.h>

extern int	_getrusage();
extern long	time();

struct timeval {
	long	tv_sec;
	long	tv_usec;
};

#define cvt(x)	(HZ *  x.tv_sec + ((HZ * x.tv_usec ) + 500000 ) / 1000000)

long
times( buffer )
	register struct tms	*buffer;	/* where to put info */
{
	struct	{
		struct timeval ru_utime;
		struct timeval ru_stime;
		int	ru_stuff[14];
		}	rusage;
	struct timeval tt;

	if ( _getrusage( 0, &rusage ) != 0 )	/* self */
		return -1L;
	buffer->tms_utime = cvt(rusage.ru_utime);
	buffer->tms_stime = cvt(rusage.ru_stime);

	if ( _getrusage( -1, &rusage ) != 0 )	/* children */
		return -1L;
	buffer->tms_cutime = cvt(rusage.ru_utime);
	buffer->tms_cstime = cvt(rusage.ru_stime);

	if (_gettimeofday(&tt, (struct timezone *)0) < 0)
		return -1L;

	return cvt(tt);
}
