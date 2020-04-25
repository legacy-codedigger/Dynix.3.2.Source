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

#ifdef RCS
static char rcsid[] = "$Header: conf.c 2.2 86/03/05 $";
#endif

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "saio.h"

devread(io)
	register struct iob *io;
{
	int cc;

	io->i_flgs |= F_RDDATA;
	io->i_error = 0;
	cc = (*devsw[io->i_ino.i_dev].dv_strategy)(io, READ);
	io->i_flgs &= ~F_TYPEMASK;
	return (cc);
}

#if !defined(BOOTXX)
devwrite(io)
	register struct iob *io;
{
	int cc;

	io->i_flgs |= F_WRDATA;
	io->i_error = 0;
	cc = (*devsw[io->i_ino.i_dev].dv_strategy)(io, WRITE);
	io->i_flgs &= ~F_TYPEMASK;
	return (cc);
}
#endif

devopen(io)
	register struct iob *io;
{
	(*devsw[io->i_ino.i_dev].dv_open)(io);
}

#if !defined(BOOTXX)
devclose(io)
	register struct iob *io;
{
	(*devsw[io->i_ino.i_dev].dv_close)(io);
}

devioctl(io, cmd, arg)
	register struct iob *io;
	int cmd;
	caddr_t arg;
{
	return ((*devsw[io->i_ino.i_dev].dv_ioctl)(io, cmd, arg));
}

devlseek(io, addr, ptr)
	register struct iob *io;
	off_t addr;
	int ptr;
{
	return ((*devsw[io->i_ino.i_dev].dv_lseek)(io, addr, ptr));
}

/*ARGSUSED*/
nullsys(io)
	struct iob *io;
{
	;
}

/*ARGSUSED*/
nullioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{
	io->i_error = ECMD;
	return (-1);
}

/*ARGSUSED*/
nulllseek(io, addr, ptr)
	struct iob *io;
	off_t addr;
	int ptr;
{
	return (-1);
}
#endif
