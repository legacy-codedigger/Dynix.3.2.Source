/*
	<sys/_ioctl.h> -- macros for ioctl codes

	last edit:	02-Dec-1984	D A Gwyn
*/

#ifndef	_IO				/* so we only do this once! */

/* NOTE:  The following definitions are not the same as those of 4.2BSD;
	  I have changed the "cls" argument to a true character constant. */

#define	_IOC( cls, sub )	(((cls) << 8) | (sub))	/* old-style ioctl code */

#ifdef	pdp11				/* normal UNIX System V definitions */

#define	_IO( cls, sub )		_IOC( cls, sub )
#define	_IOR( cls, sub, typ )	_IOC( cls, sub )
#define	_IOW( cls, sub, typ )	_IOC( cls, sub )
#define	_IOWR( cls, sub, typ )	_IOC( cls, sub )

#else					/* 4.2BSD (VAX, Gould, maybe Sun) */

#define	__IOCPARM_MASK	0x7F		/* parameters must be < 128 bytes;
					   this is an incredible lossage! */
#define	__IOC_VOID	0x20000000	/* no parameters */
#define	__IOC_OUT	0x40000000	/* copy parameters out to user space */
#define	__IOC_IN	0x80000000	/* copy parameters in from user space */
#define	__IOC_INOUT	__IOC_OUT | __IOC_IN	/* copy parameters in and out */

#define	__IOC_PARAM( typ )	((sizeof(typ) & __IOCPARM_MASK) << 16)

#define	_IO( cls, sub )		(__IOC_VOID | _IOC( cls, sub ))
#define	_IOR( cls, sub, typ )	(__IOC_OUT | _IOC( cls, sub ) | __IOC_PARAM( typ ))
#define	_IOW( cls, sub, typ )	(__IOC_IN | _IOC( cls, sub ) | __IOC_PARAM( typ ))
#define	_IOWR( cls, sub, typ )	(__IOC_INOUT | _IOC( cls, sub ) | __IOC_PARAM( typ ))

/*	native 4.2BSD TIOC[GS]ETP data structure:	*/

typedef struct
	{
	char	sg_ispeed;
	char	sg_ospeed;
	char	sg_erase;
	char	sg_kill;
	short	sg_flags;
	}	_sgttyb;		/* like sgttyb but native */

#endif

#endif
