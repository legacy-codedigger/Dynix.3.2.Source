/* @(#)$Copyright:	$
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

/* @(#)$Header: download.c 1.4 84/12/18 $ */

#include "/usr/include/stdio.h"
#include "/usr/include/sys/ioctl.h"
#include <a.out.h>			/* special, for cross environment */
#include "host.h"

/*
 * down load a file, if argument addr is zero, then assume download
 * a runnable file and modify registers, etc.  If non-zero, then it
 * is the address to download to, and no registers are modified, this
 * is to support the bootstrap stuff.
 */

download(s, addr, ptextdata, pbss)
char *s;
unsigned addr;
int *ptextdata, *pbss;
{
	register int fd, i;
	register int text, data, bss; 
	int textaddr, textentr, modreg;
#ifdef NOPE
	int sbreg;
#endif
	register bcount, checksum;
	char cc[1];
	union {
		struct exec a;
		char buf[BUFSIZ];
	} e;
	char pbuf[100];

	fd = open(s, 0);		/* open the file */
	if (fd < 0) {
		printf("%s: file %s not found\n", myname, s);
		*pbss = -1;
		goto err;
	}
	i = read(fd, e.buf, BUFSIZ);
	if (N_BADMAG(e.a)) {
		printf("%s: file %s not a.out format\n", myname, s);
		*pbss = -1;
		goto err;
	}
	if (i < sizeof e.a) {
		printf("%s: read error on file %s\n", myname, s);
		*pbss = -1;
		goto err;
	}
	text     = e.a.a_text;
	data     = e.a.a_data;
	*ptextdata = text + data;
	bss      = e.a.a_bss;
	*pbss = bss;
	textaddr = e.a.a_text_addr;
	textentr = e.a.a_entry;
	modreg	= e.a.a_entry_mod;
	errno = 0;

	/* sync with the monitor, and zero a couple of registers */
	write(port, "\r", 1);
	getprompt();
	if (addr == 0) {
		if (mflag) {
			write(port, "@wa=0\r", 6); /* clear the msr */
			getprompt();
		}
		write(port, "@cpsr=0\r", 8);	/* clear the psr */
		getprompt();
		sprintf(e.buf, "@cpc=%x\r", textentr);	/* pc */
		write(port, e.buf, strlen(e.buf));
		getprompt();
		sprintf(e.buf, "@cmo=%x\r", modreg);	/* mod */
		write(port, e.buf, strlen(e.buf));
		getprompt();
#ifdef	NOPE		/* don't try to change the mod register */
		lseek(fd, 1024+(modreg-textaddr), 0);
		if (errno) {
			sprintf(pbuf, "%s: lseek", myname);
			perror(pbuf);
			*pbss = -1;
			goto err;
		}	
		read(fd, &sbreg, 4);
		if (errno) {
			sprintf(pbuf, "%s: read", myname);
			perror(pbuf);
			*pbss = -1;
			goto err;
		}	
		sprintf(e.buf, "@csb=%x\r", sbreg);	/* sb */
		write(port, e.buf, strlen(e.buf));
		getprompt();
#endif	NOPE
							/* bss */
		sprintf(e.buf,"@f %x %x 0\r", textaddr, textaddr+text+data+bss);
	} else {
		textaddr = addr;
		sprintf(e.buf,"@f %x %x 0\r", textaddr, textaddr+text+data);
	}
	write(port, e.buf, strlen(e.buf));
	getprompt();
	lseek(fd, (long)(N_TXTOFF(e.a)), 0);
	if (errno) {
		sprintf(pbuf, "%s: lseek", myname);
		perror(pbuf);
		*pbss = -1;
		goto err;
	}
	bcount = text + data;
	printf(" %7d", bcount);
	fflush(stdout);
	while (bcount > 0) {
		i = read(fd, e.buf, BUFSIZ);
		if (i < 0) {
			printf("%s: read error on file %s\n", myname, s);
			*pbss = -1;
			goto err;
		}
		if (i > bcount)
			i = bcount;
		write(port, "@i\r", 3);	/* start the image loader */

		cc[0] = textaddr;		/* write the start address */
		write(port, cc, 1);
		cc[0] = textaddr >> 8;
		write(port, cc, 1);
		cc[0] = textaddr >> 16;
		write(port, cc, 1);
		cc[0] = textaddr >> 24;
		write(port, cc, 1);
		textaddr += BUFSIZ;		/* next write */

		cc[0] = i;			/* write the byte count */
		write(port, cc, 1);
		cc[0] = i >> 8;
		write(port, cc, 1);
		cc[0] = i >> 16;
		write(port, cc, 1);
		cc[0] = i >> 24;
		write(port, cc, 1);

		write(port, e.buf, i);	/* write the data */

		checksum = 0;
		while (--i > -1)
			checksum += e.buf[i];
		cc[0] = checksum;
		write(port, cc, 1);	/* send the checksum */
		bcount -= BUFSIZ;
		if (bcount < 0)
			bcount = 0;
		write(port, "\r", 1);
		getprompt();
		printf(" %7d", bcount);
		fflush(stdout);
	}
	write(port, "\r", 1);
err:
	fflush(stdout);
	close(fd);
}

getprompt()
{
	char cc[1];
	int npending;
	char prbuf[1000];
	register char prcnt = 0, err = 0;

	cc[0] = 0;
	while (cc[0] != '*') {
		read(port, cc, 1);
		prbuf[prcnt++] = cc[0];
		if (cc[0] == '?') {
			err++;
			printf("\r\n%s: unexpected error on remote.\r\n",
				myname);
			fflush(stdout);
		}
	}
	read(port, cc, 1);		/* read the '\r' */
	if (cc[0] == '?')
		err++;
	prbuf[prcnt++] = cc[0];
	read(port, cc, 1);		/* read the '\n' */
	if (cc[0] == '?')
		err++;
	prbuf[prcnt++] = cc[0];
	ioctl(port, FIONREAD, &npending);
	while (npending > 0) {
		read(port, cc, 1);
		if (cc[0] == '?')
			err++;
		prbuf[prcnt++] = cc[0];
		ioctl(port, FIONREAD, &npending);
	}
	if (err) {
		prbuf[prcnt] = 0;
		printf("%s\r\n", prbuf);
		fflush(stdout);
	}
}
