/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: stdio.h 2.7 1991/09/30 17:52:18 $ */

#ifndef NULL
#define	NULL	0
#endif

#ifndef FILE
#define	BUFSIZ	1024
#define	_NFILE	20

extern	struct	_iobuf {
	int	_cnt;
	char	*_ptr;		/* should be unsigned char */
	char	*_base;		/* ditto */
	int	_bufsiz;
	short	_flag;
	char	_file;		/* should be short */
} _iob[];

#define	_IOREAD	01
#define	_IOWRT	02
#define	_IONBF	04
#define	_IOMYBUF	010
#define	_IOEOF	020
#define	_IOERR	040
#define	_IOSTRG	0100
#define	_IOLBF	0200
#define	_IORW	0400
#define	FILE	struct _iobuf
#define	EOF	(-1)

#define	stdin	(&_iob[0])
#define	stdout	(&_iob[1])
#define	stderr	(&_iob[2])
#ifndef lint
#define	getc(p)		(--(p)->_cnt>=0? (int)(*(unsigned char *)(p)->_ptr++):_filbuf(p))
#endif /* not lint */
#define	getchar()	getc(stdin)
#ifndef lint
#define putc(x,p) (--(p)->_cnt>=0?\
	((int)(*(unsigned char *)(p)->_ptr++= (x))):\
		_flsbuf((unsigned char)(x),p))
#endif	/* lint */
#define	putchar(x)	putc(x,stdout)
#define	feof(p)		(((p)->_flag&_IOEOF)!=0)
#define	ferror(p)	(((p)->_flag&_IOERR)!=0)
#define	fileno(p)	((p)->_file)
#define	clearerr(p)	((p)->_flag &= ~(_IOERR|_IOEOF))

FILE	*fopen();
FILE	*fdopen();
FILE	*freopen();
FILE	*popen();
long	ftell();
char	*fgets();
char	*gets();
char	*sprintf();	/* Note in 4.3 this is int */
# endif

#ifndef __cplusplus
char	*getlogin();
#endif
