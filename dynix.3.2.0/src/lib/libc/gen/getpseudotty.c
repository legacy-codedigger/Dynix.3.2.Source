/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: getpseudotty.c 1.6 87/07/29 $ */

#include <sys/ioctl.h>
#include <sys/file.h>
#include <errno.h>

#define	GETPTY	"/dev/getpty"
#define	I1	sizeof("/dev/pty")-1
#define I2	sizeof("/dev/ptyX")-1
#define	NULL	0

static char *c_line  =	"/dev/ptyXX";
static char *s_line  =	"/dev/ttyXX";

static char LT1[] = "pqrstuvwPQRSTUVW";
static char LT2[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

#define	L1	16
#define	L2	62

/*
 * getpseudotty()
 * 	Allocate a pseudo tty pair
 *
 * Returns the fd associated with the master side.  Returns
 * -1 if no pseudo ports are available.  Also returns the 
 * name of both slave and master ports.
 */
int
getpseudotty(slave, master)
	char	**slave;
	char	**master;
{
	int	p;
	int	fd;
	int	nextpty;
	int	tries;
	int	maxminor;

	if ((p = open(GETPTY, O_RDONLY)) < 0)
		return (-1);
	maxminor = L1 * L2;
	for (tries = 0; tries < 16; tries++) {
		nextpty = maxminor;
		if (ioctl(p, TIOCGETN, (char *)&nextpty) < 0 || nextpty == -1)
			return (-1);
		c_line[I1] = LT1[ nextpty / L2 ];
		c_line[I2] = LT2[ nextpty % L2 ];
		fd = open(c_line, O_RDWR);
		if (fd >= 0) {
			s_line[I1] = c_line[I1];
			s_line[I2] = c_line[I2];
			/*
			 * Try to reclaim a line if 
			 * someone left it in a bad state.
			 */
			(void) chmod(s_line, 0666);
			if (access(s_line, R_OK|W_OK)) {
				(void) close(fd);
				continue;
			}
			*master = c_line;
			*slave = s_line;
			(void) close(p);
			return (fd);
		}
		if (errno == ENOENT)
			maxminor = (nextpty / L2) * L2;
	}
	(void) close(p);
	return (-1);
}

/*
 * ispseudotty()
 *	Answer whether a tty is a pseudo port or not
 *
 * Returns non-zero if argument is a pseudo port; else return zero.
 */
ispseudotty(s)
	register char *s;
{
	extern char *index();

	if (s != NULL && s[0] == 't' && s[1] == 't' && s[2] == 'y')
		return (index(LT1, s[3]) == NULL ? 0 : 1);
	return 0;
}
