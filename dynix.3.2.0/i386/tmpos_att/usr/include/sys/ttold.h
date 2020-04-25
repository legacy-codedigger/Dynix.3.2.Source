/* @(#)ttold.h	6.1 */
struct	sgttyb {
	char	sg_ispeed;
	char	sg_ospeed;
	char	sg_erase;
	char	sg_kill;
	int	sg_flags;	/* DAG -- important: not "short"! */
};

/* modes */
#define	O_HUPCL	01
#define	O_XTABS	02
#define	O_LCASE	04
#define	O_ECHO	010
#define	O_CRMOD	020
#define	O_RAW	040
#define	O_ODDP	0100
#define	O_EVENP	0200
#define	O_NLDELAY	001400
#define	O_NL1	000400
#define	O_NL2	001000
#define	O_TBDELAY	002000
#define	O_NOAL	004000
#define	O_CRDELAY	030000
#define	O_CR1	010000
#define	O_CR2	020000
#define	O_VTDELAY	040000
#define	O_BSDELAY	0100000

#include	<sys/_ioctl.h>

#define	tIOC		_IOC( 't', 0 )

#define	TIOCGETP	_IOR( 't', 8, _sgttyb )
#define	TIOCSETP	_IOW( 't', 9, _sgttyb )
