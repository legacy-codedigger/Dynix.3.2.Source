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
static	char rcsid[] = "$Header: genassym.c 1.4 90/10/12 $";
#endif

/*
 * Generate assembler symbols needed from C header files
 */

#include <sys/types.h>
#include <machine/cfg.h>
#include <a.out.h>

/* Stack Pointer Based */
#define	SPARG0	"4+0(%esp)"
#define	SPARG1	"4+4(%esp)"
#define	SPARG2	"4+8(%esp)"
#define	SPARG3	"4+12(%esp)"
#define	SPARG4	"4+16(%esp)"
#define	SPARG5	"4+20(%esp)"
#define	SPARG6	"4+24(%esp)"
#define	SPARG7	"4+28(%esp)"

/* Frame Pointer Based */
#define	FPARG0	"8+0(%ebp)"
#define	FPARG1	"8+4(%ebp)"
#define	FPARG2	"8+8(%ebp)"
#define	FPARG3	"8+12(%ebp)"
#define	FPARG4	"8+16(%ebp)"
#define	FPARG5	"8+20(%ebp)"
#define	FPARG6	"8+24(%ebp)"
#define	FPARG7	"8+28(%ebp)"

#define	CALL	"call"
#define	RETURN	"ret"
#define	ENTER	"pushl %ebp; movl %esp,%ebp"
#define	EXIT	"leave"

main()
{

	struct cfg_ptr *cfgptr = (struct cfg_ptr *)0;
	struct cfg_boot *boot = (struct cfg_boot *)0;
	struct config_desc *cd = (struct config_desc *)0;
	struct exec *execptr = (struct exec *)0;

	/* old firmware */
	genhex("CFG_PTR", CFG_PTR);
	genhex("head_cfg", &cfgptr->head_cfg);
	genhex("b_bottom", &boot->b_bottom);

	/* new firmware */
	genhex("CD_LOC", CD_LOC);
	genhex("c_bottom", &cd->c_bottom);
	genhex("c_version", &cd->c_version);

	genstr("SPARG0", SPARG0);
	genstr("SPARG1", SPARG1);
	genstr("SPARG2", SPARG2);
	genstr("SPARG3", SPARG3);
	genstr("SPARG4", SPARG4);
	genstr("SPARG5", SPARG5);
	genstr("SPARG6", SPARG6);
	genstr("SPARG7", SPARG7);

	genstr("FPARG0", FPARG0);
	genstr("FPARG1", FPARG1);
	genstr("FPARG2", FPARG2);
	genstr("FPARG3", FPARG3);
	genstr("FPARG4", FPARG4);
	genstr("FPARG5", FPARG5);
	genstr("FPARG6", FPARG6);
	genstr("FPARG7", FPARG7);

	genstr("CALL",	CALL);
	genstr("RETURN", RETURN);
	genstr("ENTER",	ENTER);
	genstr("EXIT",	EXIT);
}

genstr(s, t)
char *s, *t;
{
	printf("#define	%s	%s\n", s, t);
}

genhex(s, val)
char *s;
int val;
{
	printf("#define	%s	0x%x\n", s, val);
}

gendec(s, val)
char *s;
int val;
{
	printf("#define	%s	%d\n", s, val);
}
