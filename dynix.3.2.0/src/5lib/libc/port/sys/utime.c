/*
	utime -- system call emulation for 4.2BSD

	last edit:	14-Dec-1983	D A Gwyn
*/

#include	<sys/types.h>

extern int	_utimes();

#define NULL	0

typedef struct
	{
	time_t	actime;
	time_t	modtime;
	}	utimbuf;

typedef struct
	{
	unsigned long	tv_sec;		/* seconds */
	long		tv_usec;	/* microseconds */
	}	timeval;

int
utime( path, times )
	char	*path;			/* file to be updated */
	utimbuf *times; 		/* -> new access/mod times */
	{
	timeval	tv[2];

	if ( times != NULL ) {
		tv[0].tv_sec = times->actime;
		tv[0].tv_usec = 0L;
		tv[1].tv_sec = times->modtime;
		tv[1].tv_usec = 0L;
		return _utimes( path, tv );
	} else
		return _utimes( path, (timeval *)NULL );
	}
