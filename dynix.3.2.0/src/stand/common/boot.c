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
static char rcsid[] = "$Header: boot.c 2.12 90/11/06 $";
#endif

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <sys/vm.h>
#include <sys/reboot.h>
#include <machine/cfg.h>
#include <a.out.h>
#include "saio.h"
#include "filehdr.h"
#include "aouthdr.h"
#include "scnhdr.h"

caddr_t calloc();

/*
 * Boot program... flags/strings passed in configuration structures determine
 * whether boot stops to ask for system name and which device boot comes from.
 */

main()
{
	register struct config_desc *cd = CD_LOC;
	register int i;
	register char *r, *s;
	register int io;
	static	int retry;
	char	*line;

	/*
	 * Build new style configuration
	 * tables if SCED running old firmware.
	 * A nop if new firmware.
	 */
	buildcfg();

	printf("Boot\n");
	line = calloc(cd->c_bname_size);
	for (;;) {
		if (cd->c_boot_flag & RB_ASKNAME) {
			printf(": ");
			gets(s = line);
			r = cd->c_boot_name;
			/* skip leading white space */
			while (*s == ' ' || *s == '\t')
				s++;
			/* copy new line into config structure */
			for (i = cd->c_bname_size; i-- > 0; ) {
				switch (*s) {
				case '\0':	/* end of line */
					*r++ = 0;
					break;
				case '\t':	/* tab */
				case ' ':	/* space */
					*r++ = 0; s++;
					break;
				default:	/* character */
					*r++ = *s++;
					break;
				}
			}
			bcopy(cd->c_boot_name, 
			    ((struct cfg_ptr *)CFG_PTR)->head_cfg->b_boot_name, 
			    BNAMESIZE);
		} else
			printf(": %s\n", cd->c_boot_name);
		io = open(cd->c_boot_name, 0);
		if (io >= 0)
			copyunix(cd->c_boot_flag, io);
		else
			printf("Can't open %s\n", cd->c_boot_name);
		if (++retry > 2)
			cd->c_boot_flag = RB_SINGLE|RB_ASKNAME;
	}
}

/*ARGSUSED*/
copyunix(howto, io)
	int howto, io;
{
	register int n, offset;
	register struct exec *e;

	/*
	 * start reading file at 1K boundary past end of allocated memory.
	 */
	callocrnd(1024);
	offset = (int)calloc(0);
	e = (struct exec *)offset;

	/* read header and check magic number */
	n = roundup(sizeof(struct exec), DEV_BSIZE);
	if (read(io, offset, n) != n)
		goto sread;
	if (e->a_magic != SMAGIC)
		copyunix_coff(howto,io);
	/* read text */
	printf("%d", e->a_text);
	if (read(io, offset + n, e->a_text - n) != e->a_text - n)
		goto sread;

	/* read data */
	printf("+%d", n = e->a_data);
	if (read(io, offset + e->a_text, n) != n)
		goto sread;
	
	/* NB: programs clear their own BSS */
	printf("+%d start 0x%x\n", e->a_bss, e->a_entry);

	(void) close(io);

#ifdef	ns32000
	/* detect if this is an old or new standalone binary */
	for (n = sizeof(struct exec); n < CD_STAND_ADDR; n += sizeof(int)) {
		if (*(int *)(offset + n) != 0)
			break;
	}
	if (n >= CD_STAND_ADDR) {
		/* new standalone so copy configuration tables */
		bcopy(CD_LOC, offset + (int)CD_LOC, CD_STAND_ADDR - (int)CD_LOC);
	} else { 
		int entry = e->a_entry;

		printf("warning: old standalone format file.\n");

		/* save old cfg pointer into new program */
		*(struct cfg_ptr *)(offset + CFG_PTR) = *(struct cfg_ptr *)CFG_PTR;

		/* exec new program over ourselves */
		gsp(offset, 0, e->a_text + e->a_data, entry);
		_stop("exec failed");
	}
#else	not ns32000
	/* copy configuration tables */
	bcopy(CD_LOC, offset + (int)CD_LOC, CD_STAND_ADDR - (int)CD_LOC);
#endif	ns32000

	/* exec new program over ourselves */
	gsp(offset, 0, e->a_text + e->a_data, e->a_entry);
sread:
	(void) close(io);
	_stop("short read");
}


#define NSCNS	6
#define COFFHDR	sizeof(struct filehdr)+sizeof(struct aouthdr)+\
	(NSCNS*sizeof(struct scnhdr))

/*
 * Structure that includes the whole COFF header so that
 * all of the header information for the file may be read at once.
 */
struct headers {
	struct filehdr f;
	struct aouthdr a;
	struct	scnhdr	s[NSCNS];
	char	pad[roundup(COFFHDR, DEV_BSIZE) - COFFHDR];
} head;

struct scnhdr *section;


/*
 * copyunix_coff()
 *
 * load the COFF binary from io into memory.
 * Assumes that sections appear in the following order:
 * 	16 bit startup-mode code (optional)
 *	text (1 or 2 sections)
 *	data
 *	bss
 * Since ts driver can only read in DEV_BSIZE blocks, all section
 * data is read with DEV_BSIZE granularity, then the inter-section
 * overlap is cleaned up via ovbcopy().
 *
 * The only exception to this is the /boot program itself (this program).
 * Unlike all the other standalones, /boot is loaded without the
 * 16-bit startup section and with its other sections aligned on
 * DEV_BSIZE boundaries in the COFF binary file.  This allows the 8K
 * bootstrap to be simpler.
 *
 * Note: we assume that if the 16-bit startup section is absent, then
 * the rest of the section data is aligned in the COFF file.
 */

/*ARGSUSED*/
copyunix_coff(howto, io)
	int howto, io;
{
	int	length,
		n, k,
		startup,
		no_of_nscns;
	long	offset,
		entry,
		org0_size,
		org0_offset,
		text1_size,
		text1_offset,
		text1_start,
		text2_size,
		text2_offset,
		text2_start,
		data_size,
		data_offset,
		data_start,
		bss_size,
		bss_start,
		slop;
	char	slopbuf[DEV_BSIZE];

	startup = 0;
	text1_size = 0L;
	text2_size = 0L;
	slop = 0L;
	/*
	 * start reading file at 1K boundary past end of allocated memory.
	 */
	callocrnd(1024);
	offset = (long)calloc(0);
	n = roundup(COFFHDR, DEV_BSIZE);

	/* 
	 *	read headers and check magic number
	 */
	if(lseek(io,0L,0) == -1)
		_stop("Lseek failed");
	if (read(io, &head, n) != n)
		goto sread;
	if (head.f.f_magic != I386MAGIC)
		_stop("Bad a.out magic number");
	entry = head.a.entry;

	/*
	 *	read all the section headers noting
	 *	text, data, and bss information
	 */
	no_of_nscns = head.f.f_nscns;
	if (no_of_nscns > NSCNS) {
		_stop("Bad standalone utility - too many sections\n");
	}
	
	for (section = &head.s[0]; no_of_nscns-- > 0; section++) {
		if ((section->s_flags & STYP_TEXT) && section->s_vaddr == 0) {
			org0_size = section->s_size;
			org0_offset = section->s_scnptr;
			startup++;
		}
		else if ((section->s_flags & STYP_TEXT)
		     && section->s_vaddr == CD_STAND_ADDR) {
			text1_size = section->s_size;
			text1_offset = section->s_scnptr;
			text1_start = section->s_vaddr;
		}
		else if (section->s_flags & STYP_TEXT) {
			text2_size = section->s_size;
			text2_offset = section->s_scnptr;
			text2_start = section->s_vaddr;
		}
		else if (section->s_flags & STYP_DATA) {
			data_size = section->s_size;
			data_offset = section->s_scnptr;
			data_start = section->s_vaddr;
		}
		else if (section->s_flags & STYP_BSS) {
			bss_size = section->s_size;
			bss_start = section->s_vaddr;
		}
		/* ignore other sections */

		/*
		 *	Handle files with 2 text sections specially.
		 *	Namely, 16-bit-mode code and the first text
		 *	section will be loaded above the other sections
		 *	which will be loaded at their virtual address.
		 */
		if ((section->s_flags & (STYP_TEXT|STYP_DATA))
		 &&  text2_size != 0
		 &&  section->s_vaddr + section->s_size > offset) {
			offset = section->s_vaddr + section->s_size;
			offset = roundup(offset, 1024);
		}
	}

	/*
	 *	load 16-bit-mode startup code (if it's present)
	 */
	if (startup) {
		n = roundup(org0_size, DEV_BSIZE);
		if (read(io, offset, n) != n)
			goto sread;
		if (n != org0_size) {
			slop = n - org0_size;
			bcopy(offset + org0_size, slopbuf, slop);
		}
		else
			slop = 0;
	}

	/*
	 *	load text image
	 */
	printf("%d", text1_size + text2_size);
	if (slop) {
		bcopy(slopbuf, offset + text1_start, slop);
	}
	k = roundup(text1_size - slop, DEV_BSIZE);
	if (read(io, offset + text1_start + slop, k) != k)
		goto sread;
	if (k != text1_size - slop) {
		slop = k - (text1_size - slop);
		bcopy(offset + text1_start + text1_size, slopbuf, slop);
	}
	else
		slop = 0;
	if (text2_size) {
		if (slop) {
			bcopy(slopbuf, text2_start, slop);
		}
		k = roundup(text2_size - slop, DEV_BSIZE);
		if (read(io, text2_start + slop, k) != k)
			goto sread;
		if (k != text2_size - slop) {
			slop = k - (text2_size - slop);
			bcopy(text2_start + text2_size, slopbuf, slop);
		}
		else
			slop = 0;
	}

	/*
	 *	load data image
	 */
	printf("+%d", data_size);
	if (slop) {
		bcopy(slopbuf, (text2_size ? 0L : offset) + data_start, slop);
	}
	k = roundup(data_size - slop, DEV_BSIZE);
	if (read(io, (text2_size ? 0L : offset) + data_start + slop, k) != k)
		goto sread;

	printf("+%d start 0x%x\n", bss_size, entry);

	(void) close(io);

	/*
	 *	copy configuration tables
	 */
	bcopy(CD_LOC, offset + (int)CD_LOC, CD_STAND_ADDR - (int)CD_LOC);

	/*
	 *	exec new program over ourselves
	 */
	if (text2_size)
		length = text1_start + text1_size;
	else
		length = data_start + data_size;
	gsp(offset, 0, length, entry);
	/*NOTREACHED*/
sread:
	_stop("short read");
}
