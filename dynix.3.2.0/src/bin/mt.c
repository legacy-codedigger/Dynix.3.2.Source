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

#ifndef lint
static char rcsid[] = "$Header: mt.c 2.5 1991/05/23 17:55:27 $";
#endif

/*
 * mt --
 *   magnetic tape manipulation program
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <errno.h>

#define	equal(s1,s2)	(strcmp(s1, s2) == 0)

struct commands {
	char *c_name;
	int c_code;
	int c_ronly;
} com[] = {
	{ "weof",	MTWEOF,	0 },
	{ "eof",	MTWEOF,	0 },
#if defined(sequent)
	{ "erase",	MTERASE, 0},
#endif
	{ "fsf",	MTFSF,	1 },
	{ "bsf",	MTBSF,	1 },
	{ "fsr",	MTFSR,	1 },
	{ "bsr",	MTBSR,	1 },
	{ "rewind",	MTREW,	1 },
	{ "offline",	MTOFFL,	1 },
	{ "rewoffl",	MTOFFL,	1 },
#if defined(sequent)
	{ "ret",	MTRET,	1 },
	{ "retension",	MTRET,	1 },
	{ "eod",	MTSEOD,	1 },
	{ "seod",	MTSEOD, 1 },
	{ "noret",	MTNORET, 1 },
#endif
	{ "status",	MTNOP,	1 },
	{ 0 }
};

int mtfd;
struct mtop mt_com;
struct mtget mt_status;
char *tape;

main(argc, argv)
	char **argv;
{
	char line[80], *getenv();
	register char *cp;
	register struct commands *comp;

	if (argc > 2 && (equal(argv[1], "-t") || equal(argv[1], "-f"))) {
		argc -= 2;
		tape = argv[2];
		argv += 2;
	} else
		if ((tape = getenv("TAPE")) == NULL)
			tape = DEFTAPE;
	if (argc < 2)
		usage();
	cp = argv[1];
	for (comp = com; comp->c_name != NULL; comp++)
		if (strncmp(cp, comp->c_name, strlen(cp)) == 0)
			break;
	if (comp->c_name == NULL)
		usage();
	if ((mtfd = open(tape, comp->c_ronly ? 0 : 2)) < 0) {
		perror(tape);
		exit(1);
	}
	if (comp->c_code != MTNOP) {
		mt_com.mt_op = comp->c_code;
		mt_com.mt_count = (argc > 2 ? atoi(argv[2]) : 1);
		if (mt_com.mt_count < 0) {
			fprintf(stderr, "mt: negative repeat count\n");
			exit(1);
		}
		if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0) {
			fprintf(stderr, "%s %s %d ", tape, comp->c_name,
				mt_com.mt_count);
			perror("failed");
			exit(2);
		}
	} else {
		if (ioctl(mtfd, MTIOCGET, (char *)&mt_status) < 0) {
			if (errno == EINVAL)
				fprintf(stderr, "%s does not support the \"%s\" command\n",
					tape, comp->c_name);
					
			else
				perror("mt");
			exit(2);
		}
		status(&mt_status);
	}
}

#ifdef vax
#include <vaxmba/mtreg.h>
#include <vaxmba/htreg.h>

#include <vaxuba/utreg.h>
#include <vaxuba/tmreg.h>
#undef b_repcnt		/* argh */
#include <vaxuba/tsreg.h>
#endif

/* TODO:  Move these to sys directory when integrating 9-track tape */
#if defined(sequent)
#define TSDS_BITS	"\20\16NDT\14BOT\7EOT\5WRP\4EOM\1FIL"
#define TSER_BITS	"\20\13BPE\7CNI\3UDE\2BNL"
#define	XTDS_BITS	"\20\10EOT\6FPT\5REW\4ONL\3RDY\2DBY\1FBY"
#define	XTER_BITS	"\20\10ERRS\1DONE"
#endif

#ifdef sun
#include <sundev/tmreg.h>
#include <sundev/arreg.h>
#endif

struct tape_desc {
	short	t_type;		/* type of magtape device */
	char	*t_name;	/* printing name */
	char	*t_dsbits;	/* "drive status" register */
	char	*t_erbits;	/* "error" register */
} tapes[] = {
#ifdef vax
	{ MT_ISTS,	"ts11",		0,		TSXS0_BITS },
	{ MT_ISHT,	"tm03",		HTDS_BITS,	HTER_BITS },
	{ MT_ISTM,	"tm11",		0,		TMER_BITS },
	{ MT_ISMT,	"tu78",		MTDS_BITS,	0 },
	{ MT_ISUT,	"tu45",		UTDS_BITS,	UTER_BITS },
#endif
#ifdef sun
	{ MT_ISCPC,	"TapeMaster",	TMS_BITS,	0 },
	{ MT_ISAR,	"Archive",	ARCH_CTRL_BITS,	ARCH_BITS },
#endif
#if defined(sequent)
	{ MT_ISXT, 	"(Xylogics controller)",	XTDS_BITS,	XTER_BITS },
	{ MT_ISTS,	"Scsi streamer",	TSDS_BITS,	TSER_BITS },
	{ MT_ISTB,	"Scsi 9 track",		TSDS_BITS,	TSER_BITS },
#endif
	{ 0 }
};

/*
 * Interpret the status buffer returned
 */
status(bp)
	register struct mtget *bp;
{
	register struct tape_desc *mt;

	for (mt = tapes; mt->t_type; mt++)
		if (mt->t_type == bp->mt_type)
			break;
	if (mt->t_type == 0) {
		printf("unknown tape drive type (%d)\n", bp->mt_type);
		return;
	}
	printf("%s tape drive, residual=%d\n", mt->t_name, bp->mt_resid);
	printreg("ds", bp->mt_dsreg, mt->t_dsbits);
	printreg("\ner", bp->mt_erreg, mt->t_erbits);
	putchar('\n');
	printf("fileno %d, blkno %d\n", bp->mt_fileno, bp->mt_blkno);
}

/*
 * Print a register a la the %b format of the kernel's printf
 */
printreg(s, v, bits)
	char *s;
	register char *bits;
	register unsigned short v;
{
	register int i, any = 0;
	register char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (v && bits) {
		putchar('<');
		while (i = *bits++) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

usage()
{
	register struct commands *comp;
	register count = 0;

	fprintf(stderr, "usage: mt [ -f device ] command [ count ]\nwhere command is one of:\n");
	for(comp = com; comp->c_name != NULL; comp++) {
		fprintf(stderr, "%s%s", 
	(comp == com || ((count % 10) == 0)) ? " " : ", ", comp->c_name);
		++count;
		if ((count % 10) == 0)
			fprintf(stderr, ",\n");
	}
	putc('\n', stderr);
	exit(1);
}
