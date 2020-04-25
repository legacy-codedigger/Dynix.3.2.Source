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
static char rcsid[] = "$Header: bootxx.c 2.7 87/07/28 $";
#endif

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <machine/cfg.h>
#include <a.out.h>
#include "saio.h"

/*
 * 8K byte boot program that loads /boot.
 */
main()
{
	register int io;
	register char *s;
	struct cfg_boot *boot = ((struct cfg_ptr *)CFG_PTR)->head_cfg;
	char bootprog[BNAMESIZE];
	char *index();

	bcopy(boot->b_boot_name, bootprog, BNAMESIZE);
	s = index(bootprog, ')') + 1;
	if (devsw[0].dv_flags & D_TAPE)
		strcpy(index(bootprog, ','), ",1)");	/* file1 for tapes */
	else
		strcpy(s, "boot");

	printf("loading %s\n", bootprog);
	io = open(bootprog, 0);
	if (io >= 0)
		copyunix(io);
	_stop("boot failed");
}

/* to make bootstraps smaller */
exit() 
{
}

/* to make bootstraps smaller */
_stop(s)
	char *s;
{

	printf("%s\n", s);
	for(;;)
		/* spin */;
}

copyunix(io)
	register io;
{
	register int n, offset;
	register struct exec *e;
	int entry;
	caddr_t	calloc();

	/*
	 * start reading file at 1K boundary past end of allocated memory.
	 */
	callocrnd(1024);
	offset = (int)calloc(0);
	e = (struct exec *)offset;

	/* read a.out header and check magic number */
	n = roundup(sizeof(struct exec), DEV_BSIZE);
	if (read(io, offset, n) != n)
		goto sread;
	if (e->a_magic != SMAGIC) 
		_stop("Bad a.out magic number");

	/* read text */
	if (read(io, offset + n, e->a_text - n) != e->a_text - n)
		goto sread;

	/* read data */
	n = e->a_data;
	if (read(io, offset + e->a_text, n) != n)
		goto sread;

	/* grab entry before overwritten by cfg_ptr */
	entry = e->a_entry;

	/* save old cfg pointer into new program */
	*(struct cfg_ptr *)(offset + CFG_PTR) = *(struct cfg_ptr *)CFG_PTR;

	/* copy configuration tables */
	bcopy(CD_LOC, offset + (int)CD_LOC, CD_STAND_ADDR - (int)CD_LOC);

	/* exec new program over ourselves */
	gsp(offset, 0, e->a_text + e->a_data, entry);
sread:
	_stop("short read");
}
