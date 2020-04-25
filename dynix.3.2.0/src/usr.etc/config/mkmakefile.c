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

#ifndef lint
static char rcsid[] = "$Header: mkmakefile.c 2.9 90/11/12 $";
#endif

/*
 * Build the Makefile for the system, from
 * the information in the files files and the
 * additional files for the machine being compiled to.
 */

#include <stdio.h>
#include <ctype.h>
#include "y.tab.h"
#include "config.h"

#define next_word(fp, wd) \
	{ register char *word = get_word(fp); \
	  if (word == (char *)EOF) \
		return; \
	  else \
		wd = word; \
	}

static	struct file_list *fcur;
char *tail();

/*
 * Lookup a file, by make.
 */
struct file_list *
fl_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(fp->f_fn, file))
			return (fp);
	}
	return (0);
}

/*
 * Lookup a file, by final component name.
 */
struct file_list *
fltail_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(tail(fp->f_fn), tail(file)))
			return (fp);
	}
	return (0);
}

/*
 * Make a new file list entry
 */
struct file_list *
new_fent()
{
	register struct file_list *fp;

	fp = (struct file_list *) malloc(sizeof *fp);
	fp->f_needs = 0;
	fp->f_next = 0;
	fp->f_flags = 0;
	fp->f_type = 0;
	if (fcur == 0)
		fcur = ftab = fp;
	else
		fcur->f_next = fp;
	fcur = fp;
	return (fp);
}

char	*COPTS;

/*
 * Build the makefile from the skeleton
 */
makefile()
{
	FILE *ifp, *ofp;
	char line[BUFSIZ];
	struct opt *op;

	read_files();
	strcpy(line, "../conf/Makefile.");
	if (oldfw)
		(void) strcat(line, "oldfw");
	else
		(void) strcat(line, machinename);
	ifp = fopen(line, "r");
	if (ifp == 0) {
		perror(line);
		exit(1);
	}
	ofp = fopen(path("Makefile"), "w");
	if (ofp == 0) {
		perror(path("Makefile"));
		exit(1);
	}
	fprintf(ofp, "IDENT=-D%s", raise(ident));
	if (profiling)
		fprintf(ofp, " -DGPROF");
	if (cputype == 0) {
		printf("cpu type must be specified\n");
		exit(1);
	}
	{ struct cputype *cp;
	  for (cp = cputype; cp; cp = cp->cpu_next)
		fprintf(ofp, " -D%s", cp->cpu_name);
	}
	for (op = opt; op; op = op->op_next)
		if (op->op_value)
			fprintf(ofp, " -D%s=\"%s\"", op->op_name, op->op_value);
		else
			fprintf(ofp, " -D%s", op->op_name);
	fprintf(ofp, "\n");
	if (hadtz == 0)
		printf("timezone not specified; gmt assumed\n");
	if (maxusers == 0) {
		printf("maxusers not specified; 16 assumed\n");
		maxusers = 16;
	} else if (maxusers < 8) {
		printf("minimum of 8 maxusers assumed\n");
		maxusers = 8;
	} else if (maxusers > 512) {
		printf("maxusers truncated to 512\n");
		maxusers = 512;
	}
	fprintf(ofp, "PARAM=-DTIMEZONE=%d -DDST=%d -DMAXUSERS=%d\n",
	    timezone, dst, maxusers);
	while (fgets(line, BUFSIZ, ifp) != 0) {
		if (*line == '%')
			goto percent;
		if (profiling && strncmp(line, "COPTS=", 6) == 0) {
			register char *cp;

			fprintf(ofp, 
			    "GPROF.EX=/usr/src/lib/libc/%s/csu/gmon.ex\n",
			    machinename);
			cp = index(line, '\n');
			if (cp)
				*cp = 0;
			cp = line + 6;
			while (*cp && (*cp == ' ' || *cp == '\t'))
				cp++;
			COPTS = malloc((unsigned)(strlen(cp) + 1));
			if (COPTS == 0) {
				printf("config: out of memory\n");
				exit(1);
			}
			strcpy(COPTS, cp);
			fprintf(ofp, "%s -pg\n", line);
			continue;
		}
		fprintf(ofp, "%s", line);
		continue;
	percent:
		if (eq(line, "%FASTOBJS\n"))
			do_fastobjs(ofp);
		else if (eq(line, "%SFILES\n"))
			do_sfiles(ofp);
		else if (eq(line, "%OBJS\n"))
			do_objs(ofp);
		else if (eq(line, "%SRCOBJS\n"))
			do_srcobjs(ofp);
		else if (eq(line, "%CFILES\n"))
			do_cfiles(ofp);
		else if (eq(line, "%RULES\n"))
			do_rules(ofp);
		else if (eq(line, "%LOAD\n"))
			do_load(ofp);
		else if (eq(line, "%IOCHF\n"))
			do_iochf(ofp);
		else
			fprintf(stderr,
			    "Unknown %% construct in generic Makefile: %s",
			    line);
	}
	(void) fclose(ifp);
	(void) fclose(ofp);
}

/*
 * Read in the information about files used in making the system.
 * Store it in the ftab linked list.
 */
read_files()
{
	FILE *fp;
	register struct file_list *tp;
	register struct device *dp;
	char *wd, *this, *needs, *devorprof;
	char fname[32];
	int nreqs, first = 1, configdep, fastobj, sedit, src, nopt;

	ftab = 0;
	(void) strcpy(fname, "files");
openit:
	fp = fopen(fname, "r");
	if (fp == 0) {
		perror(fname);
		exit(1);
	}
next:
	/*
	 * filename	[ standard | optional ] 
	 *	[ config-dependent | fastobj | sedit | src | nopt ]
	 *	[ dev* | profiling-routine ] [ device-driver]
	 */
	wd = get_word(fp);
	if (wd == (char *)EOF) {
		(void) fclose(fp);
		if (first == 1) {
			if (oldfw)
				(void) sprintf(fname, "files.oldfw");
			else
				(void) sprintf(fname, "files.%s", machinename);
			first++;
			goto openit;
		}
		if (first == 2) {
			(void) sprintf(fname, "files.%s", raise(ident));
			first++;
			fp = fopen(fname, "r");
			if (fp != 0)
				goto next;
		}
		return;
	}
	if (wd == 0)
		goto next;
	this = ns(wd);
	next_word(fp, wd);
	if (wd == 0) {
		printf("%s: No type for %s.\n",
		    fname, this);
		exit(1);
	}
	if (fl_lookup(this)) {
		printf("%s: Duplicate file %s.\n",
		    fname, this);
		exit(1);
	}
	tp = 0;
	if (first == 3 && (tp = fltail_lookup(this)) != 0)
		printf("%s: Local file %s overrides %s.\n",
		    fname, this, tp->f_fn);
	nreqs = 0;
	devorprof = "";
	needs = (char *)0;
	nopt = src = sedit = fastobj = configdep = 0;
	if (eq(wd, "standard"))
		goto checkdev;
	if (!eq(wd, "optional")) {
		printf("%s: %s must be optional or standard\n", fname, this);
		exit(1);
	}
nextopt:
	next_word(fp, wd);
	if (wd == 0)
		goto doneopt;
	if (eq(wd, "config-dependent")) {
		configdep++;
		goto nextopt;
	}
	if (eq(wd, "fastobj")) {
		fastobj++;
		goto nextopt;
	}
	if (eq(wd, "sedit")) {
		sedit++;
		goto nextopt;
	}
	if (eq(wd, "src")) {
		src++;
		goto nextopt;
	}
	if (eq(wd, "nopt")) {
		nopt++;
		goto nextopt;
	}
	devorprof = wd;
	if (eq(wd, "device-driver") || eq(wd, "profiling-routine")) {
		next_word(fp, wd);
		goto save;
	}
	nreqs++;
	if (needs == 0)
		needs = ns(wd);
	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (eq(dp->d_name, wd))
			goto nextopt;
	if (eq("generic", wd) && eq("generic", wd))
		goto nextopt;
	while ((wd = get_word(fp)) != 0)
		;
	if (tp == 0)
		tp = new_fent();
	tp->f_fn = this;
	tp->f_type = INVISIBLE;
	tp->f_needs = needs;
	goto next;

doneopt:
	if (nreqs == 0) {
		printf("%s: what is %s optional on?\n",
		    fname, this);
		exit(1);
	}

checkdev:
	if (wd) {
		next_word(fp, wd);
		if (wd) {
			if (eq(wd, "config-dependent")) {
				configdep++;
				goto checkdev;
			}
			if (eq(wd, "fastobj")) {
				fastobj++;
				goto checkdev;
			}
			if (eq(wd, "sedit")) {
				sedit++;
				goto checkdev;
			}
			if (eq(wd, "src")) {
				src++;
				goto checkdev;
			}
			if (eq(wd, "nopt")) {
				nopt++;
				goto checkdev;
			}
			devorprof = wd;
			next_word(fp, wd);
		}
	}

save:
	if (wd) {
		printf("%s: syntax error describing %s\n",
		    fname, this);
		exit(1);
	}
	if (eq(devorprof, "profiling-routine") && profiling == 0)
		goto next;
	if (tp == 0)
		tp = new_fent();
	tp->f_fn = this;
	if (eq(devorprof, "device-driver"))
		tp->f_type = DRIVER;
	else if (eq(devorprof, "profiling-routine"))
		tp->f_type = PROFILING;
	else
		tp->f_type = NORMAL;
	tp->f_flags = 0;
	if (configdep)
		tp->f_flags |= CONFIGDEP;
	if (fastobj)
		tp->f_flags |= FASTOBJ;
	if (sedit)
		tp->f_flags |= SEDIT;
	if (src)
		tp->f_flags |= SRC;
	if (nopt)
		tp->f_flags |= NOPT;
	tp->f_needs = needs;
	goto next;
}

do_objs(fp)
	FILE *fp;
{
	register struct file_list *tp, *fl;
	register int lpos, len;
	register char *cp, och, *sp;
	char swapname[32];

	fprintf(fp, "OBJS=");
	if (!srcconfig) {
		fprintf(fp, "\tOBJSlib.a\n");
		return;
	}
	lpos = 6;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if ((tp->f_flags & FASTOBJ) == FASTOBJ)
			continue;
		if ((tp->f_flags & SRC) == SRC)
			continue;
		sp = tail(tp->f_fn);
		for (fl = conf_list; fl; fl = fl->f_next) {
			if (fl->f_type != SWAPSPEC)
				continue;
			sprintf(swapname, "swap%s.c", fl->f_fn);
			if (eq(sp, swapname))
				goto cont;
		}
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", sp);
		lpos += len + 1;
		*cp = och;
cont:
		;
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_srcobjs(fp)
	FILE *fp;
{
	register struct file_list *tp, *fl;
	register int lpos, len;
	register char *cp, och, *sp;
	char swapname[32];

	fprintf(fp, "SRCOBJS=");
	lpos = 9;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if ((tp->f_flags & FASTOBJ) == FASTOBJ)
			continue;
		if ((tp->f_flags & SRC) != SRC)
			continue;
		sp = tail(tp->f_fn);
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", sp);
		lpos += len + 1;
		*cp = och;
cont:
		;
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_fastobjs(fp)
	FILE *fp;
{
	register struct file_list *tp, *fl;
	register int lpos, len;
	register char *cp, och, *sp;
	char swapname[32];

	fprintf(fp, "FASTOBJS=");
	lpos = 10;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if ((tp->f_flags & FASTOBJ) != FASTOBJ)
			continue;
		sp = tail(tp->f_fn);
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", sp);
		lpos += len + 1;
		*cp = och;
cont:
		;
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_cfiles(fp)
	FILE *fp;
{
	register struct file_list *tp;
	register int lpos, len;

	fprintf(fp, "CFILES=");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 'c')
			continue;
		if (!srcconfig && (tp->f_flags & SRC) != SRC)
			continue;
		if ((len = 3 + strlen(tp->f_fn)) + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "../%s ", tp->f_fn);
		lpos += len + 1;
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_sfiles(fp)
	FILE *fp;
{
	register struct file_list *tp;
	register int lpos, len;

	fprintf(fp, "SFILES=");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 's')
			continue;
		if (!srcconfig && (tp->f_flags & SRC) != SRC)
			continue;
		if ((len = 3 + strlen(tp->f_fn)) + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "../%s ", tp->f_fn);
		lpos += len + 1;
	}
	if (lpos != 8)
		putc('\n', fp);
}

char *
tail(fn)
	char *fn;
{
	register char *cp;

	cp = rindex(fn, '/');
	if (cp == 0)
		return (fn);
	return (cp+1);
}

/*
 * Create the makerules for each file
 * which is part of the system.
 * Devices are processed with the special c2 option -i
 * which avoids any problem areas with i/o addressing
 * when assembler files are processed by as.
 *
 * C SEDIT rule uses -SO assembler flag to avoid .s temp file.
 */
do_rules(f)
	FILE *f;
{
	register char *cp, *np, och, *tp;
	register struct file_list *ftp;
	char *extras;

for (ftp = ftab; ftp != 0; ftp = ftp->f_next) {
	if (ftp->f_type == INVISIBLE)
		continue;
	if (!srcconfig && (ftp->f_flags & SRC) != SRC)
		continue;
	cp = (np = ftp->f_fn) + strlen(ftp->f_fn) - 1;
	och = *cp;
	*cp = '\0';
	fprintf(f, "%so: ../%s%c", tail(np), np, och);
	tp = tail(np);
	if (och == 's') {
		switch(machine) {
			case MACHINE_BALANCE:
				fprintf(f, " assym.h ../machine/asm.sed\n");
				fprintf(f, "\t${CPP} ${CPPOPTS} ../%ss | ${SEDCMD} | ${AS} ${SAFLAGS} -o %so\n\n", np, tp);
				break;
			default:
				fprintf(f, "\n\t${AS} -o %so ../%ss\n\n", tp, np);
				break;
		}
		continue;
	}
	fprintf(f, "\n");
	if (ftp->f_flags & CONFIGDEP)
		extras = "${PARAM} ";
	else
		extras = "";
	switch (ftp->f_type) {

	case NORMAL:

		if (ftp->f_flags & SEDIT) {
			fprintf(f, "\t${CC} -SO ${COPTS} %s../%sc | \\\n", extras, np);
			fprintf(f, "\t${SEDCMD} | ${C2} | ${AS} ${CAFLAGS} -o %so\n\n", tp);
		} else if (ftp->f_flags & NOPT) {
			fprintf(f, "\t${CC} -c ${COPTS} %s../%sc\n\n",
				extras, np);
		} else {
			fprintf(f, "\t${CC} -c -O ${COPTS} %s../%sc\n\n",
				extras, np);
		}
		break;

	case DRIVER:
		if (ftp->f_flags & SEDIT) {
			fprintf(f, "\t${CC} -SO ${COPTS} %s../%sc | \\\n", extras, np);
			fprintf(f, "\t${SEDCMD} | ${C2} -i | ${AS} ${CAFLAGS} -o %so\n\n", tp);
		} else if (ftp->f_flags & NOPT) {
			fprintf(f, "\t${CC} -c -i ${COPTS} %s../%sc\n\n",
				extras, np);
		} else {
			fprintf(f, "\t${CC} -c -O -i ${COPTS} %s../%sc\n\n",
				extras, np);
		}
		break;

	case PROFILING:
		if (!profiling)
			continue;
		if (COPTS == 0) {
			fprintf(stderr,
			    "config: COPTS undefined in generic Makefile");
			COPTS = "";
		}
		fprintf(stderr,
		    "config: don't know how to profile kernel on balance\n");
		break;

	default:
		printf("Don't know rules for %s\n", np);
		break;
	}
	*cp = och;
}
}

/*
 * Create the load strings
 */
do_load(f)
	register FILE *f;
{
	register struct file_list *fl;
	int first = 1;
	struct file_list *do_systemspec();

	fl = conf_list;
	while (fl) {
		if (fl->f_type != SYSTEMSPEC) {
			fl = fl->f_next;
			continue;
		}
		fl = do_systemspec(f, fl, first);
		if (first)
			first = 0;
	}
	if (srcconfig) {
		if (oldfw) {
			fprintf(f, "swapgeneric.o: ../machine/swapgeneric.c\n");
			fprintf(f, "\t${CC} -c -O ${COPTS} ../machine/swapgeneric.c\n\n");
		} else {
			fprintf(f, "swapgeneric.o: ../%s/swapgeneric.c\n",
								machinename);
			fprintf(f, "\t${CC} -c -O ${COPTS} ../%s/swapgeneric.c\n\n",
								machinename);
		}
	}
	fprintf(f, "all:");
	for (fl = conf_list; fl != 0; fl = fl->f_next)
		if (fl->f_type == SYSTEMSPEC)
			fprintf(f, " %s", fl->f_needs);
	fprintf(f, "\n");
}


/*
 * Create the io header file dependency list for ioconf.c
 */
do_iochf(f)
	register FILE *f;
{
	struct proto *ptp;

	fprintf(f, "IOCHF=");
	for (ptp = ptab; ptp != NULL; ptp = ptp->p_next)
		if (ptp->p_seen_flag) {
			if (srcconfig)
				fprintf(f, " ../../common/%s/ioconf.h", 
					ptp->p_name);
			else
				fprintf(f, " ../%s/ioconf.h", ptp->p_name);
		}
	fprintf(f, "\n");
}

struct file_list *
do_systemspec(f, fl, first)
	FILE *f;
	register struct file_list *fl;
	int first;
{

	fprintf(f, "%s: compiler_check rest_of_%s\n", fl->f_needs, fl->f_needs);
	fprintf(f, "rest_of_%s:& Makefile ${DEBUGLIBS} ${FASTOBJS} conf.o param.o vers.o",
		fl->f_needs);
	fprintf(f, " ioconf.o swap%s.o ", fl->f_fn);
	if (eq("generic", fl->f_fn))
		fprintf(f, "conf_generic.o ");
	fprintf(f, "${SRCOBJS} ${OBJS} symbols.sort\n");
	fprintf(f, "\t@echo loading %s\n\t@rm -f %s\n",
	    fl->f_needs, fl->f_needs);
	if (first) {
		fprintf(f, "\tsh ../conf/newvers.sh\n");
		fprintf(f, "\t${CC} ${CFLAGS} -c vers.c\n");
	}
	fprintf(f, "\t${LD} -o %s -k -e start ${LDFLAGS} ", fl->f_needs);
	fprintf(f, "${FASTOBJS} \\\n\tconf.o vers.o param.o ioconf.o ");
	fprintf(f, "swap%s.o ", fl->f_fn);
	if (eq("generic", fl->f_fn))
		fprintf(f, "conf_generic.o ");
	fprintf(f, "${SRCOBJS} ${OBJS} ${DEBUGLIBS}\n");
	fprintf(f, "\t@echo rearranging symbols\n");
	fprintf(f, "\t-${SYMORDER} symbols.sort %s\n", fl->f_needs);
	fprintf(f, "\t${SIZE} %s\n", fl->f_needs);
	fprintf(f, "\t@chmod 755 %s\n\n", fl->f_needs);
	do_swapspec(f, fl->f_fn);
	for (fl = fl->f_next; fl->f_type == SWAPSPEC; fl = fl->f_next)
		;
	return (fl);
}

do_swapspec(f, name)
	FILE *f;
	register char *name;
{

	if (!eq(name, "generic")) {
		fprintf(f, "swap%s.o: swap%s.c\n", name, name);
		fprintf(f, "\t${CC} -c -O ${COPTS} swap%s.c\n\n", name);
		return;
	}

	fprintf(f, "conf_generic.o: ../conf/conf_generic.c\n");
	fprintf(f, "\t${CC} -c -O ${COPTS} ../conf/conf_generic.c\n\n");
}

char *
raise(str)
	register char *str;
{
	register char *cp = str;

	while (*str) {
		if (islower(*str))
			*str = toupper(*str);
		str++;
	}
	return (cp);
}
