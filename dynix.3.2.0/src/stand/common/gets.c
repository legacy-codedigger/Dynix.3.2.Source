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
static char rcsid[] = "$Header: gets.c 2.3 90/11/08 $";
#endif

/*
 * This will capture a line of text typed on the console up
 * to a newline.  This echos and will handle erase and kill for line editing.
 */

static int _crt;
static char _erase;
static char _kill;

#include <sys/types.h>
#include <machine/cfg.h>
#include <machine/slicreg.h>
/*
 * Set _crt, _erase and _kill from SCED fw tables
 */
static
set_chars()
{
	register struct config_desc *cd = CD_LOC;
	static int done;
	u_char console;
	
	if ( done )
		return;
       if (cd->c_cons->cd_type == SLB_SCSIBOARD)  /* SCED */
	       console = cd->c_cons->cd_sc_cons;
       else { /* SSM */
	      console = cd->c_cons->cd_ssm_cons;
       }

	_kill = cd->c_kill;		/* get kill character */
	_erase = cd->c_erase;		/* get erase character */
	switch ( console ) {    /* is console port a crt? */
	case CDSC_LOCAL:
		_crt = (cd->c_flags & CFG_PORT0_CRT);
		break;
	case CDSC_REMOTE:
		_crt = (cd->c_flags & CFG_PORT1_CRT);
		break;
	}
	done = 1;
}

char *
gets(p)
register char *p;
{
	register int c, echoflag = 1, erasing = 0;
	char *arg = p;
	char *s = p;

#define ENDERASE if (erasing && _crt == 0) {\
			putchar('/');\
			erasing = 0;\
		 }

	set_chars();
	for (;;) {
		c = getchar();
		c &= 0x7f;		/* strip to 7 bits */
		if (c == '\t')		/* convert tabs to spaces */
			c = ' ';
		if (c == ('R' & 0x1f)) {		/* break */
			ENDERASE;
			*p = 0;
			printf("^R\n");
			for (p = s; *p; ++p) {
				if (*p < ' ')
					printf("^%c", *p + '@');
				else if (*p == '\177')
					printf("^?");
				else
					putchar(*p);
			}
			continue;
		}
		if (c == '\n' || c == '\r') {		/* end of line */
			ENDERASE;
			if (echoflag)
				putchar('\n');
			*p = 0;
			return(arg);
		}
		if (c == _erase) {		/* erase char */
			if (p > s) {
				p--;
				if (echoflag) {
					if (_crt) {
						if (*p < ' ' || *p == '\177')
							printf("\b\b  \b\b");
						else
							printf("\b \b");
					} else {
						if (!erasing) {
							putchar('\\');
							erasing++;
						}
						if (*p < ' ')
							printf("^%c", *p + '@');
						else if (*p == '\177')
							printf("^?");
						else
							putchar(*p);
					}
				}
			}
			continue;
		}
		if (c == _kill) {		/* kill char */
			while (p > s) {
				p--;
				if (echoflag && (_crt)) {
					if (*p < ' ' || *p == '\177')
						printf("\b\b  \b\b");
					else
						printf("\b \b");
				}
			}
			ENDERASE;
			if (echoflag && (_crt) == 0)
				printf(" *del*\n");
			continue;
		}
		ENDERASE;
		if (echoflag) {			/* default chars */
			if (c < ' ')
				printf("^%c", c + '@');
			else if (c == '\177')
				printf("^?");
			else
				putchar(c);
		}
		*p++ = c;
	}
}
