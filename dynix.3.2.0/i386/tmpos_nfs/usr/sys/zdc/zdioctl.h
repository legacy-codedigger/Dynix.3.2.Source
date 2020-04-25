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

/*
 * "$Header: zdioctl.h 1.4 90/07/23 $"
 */

#ifndef	_SYS_ZDIOCTL_H_

/*
 * zdioctl.h
 *	definitions of structures and defines used for ioctls 
 */

/* $Log:	zdioctl.h,v $
 */

/*
 * ZDC Ioctl commands
 */
#define ZIOC		'z'
/*
 * Already in ioctl.h
 * #define ZIOCBCMD	_IOW(z, 0, struct cb)
 */
#define ZIOSEVERE	_IO(z, 1)
#define ZIONSEVERE	_IO(z, 2)
#define ZIODEVDATA	_IOR(z, 3, struct zddev)
#define ZIOSETBBL	_IO(z, 4) 
#define ZIOGERR		_IOR(z, 5, int)
#define ZIOSETSTATE	_IOW(z, 6, char)
#define ZIOFORMATF	_IO(z, 7)
#define ZIORESERVE	_IO(z, 8)
#define ZIORELEASE	_IO(z, 9)
#ifdef DEBUG
#define ZIOCSHUTDOWN	_IO(z, 255)
#define ZIOCISHUT	_IO(z, 254)
#define ZIOC_STAT_TEST	_IO(z, 253)
#endif /* DEBUG */

/*
 * structure returned for ZIODEVDATA ioctl
 */
struct zddev {
	u_char	zd_drive;	/* drive # on controller */
	u_char  zd_ctlr;	/* controller number */
	u_char	zd_cfg;		/* probed state of device */
	u_char	zd_state;	/* driver state of device */
};

#define	_SYS_ZDIOCTL_H_
#endif	/* _SYS_ZDIOCTL_H_ */
