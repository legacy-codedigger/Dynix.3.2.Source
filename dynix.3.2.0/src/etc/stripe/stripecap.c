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
static char rcsid[] = "$Header: stripecap.c 1.2 1991/07/03 23:26:39 $";
#endif  lint

/*
 * stripecap 
 *	routines for dealing with the stripe filesystem data base.
 *
 * BUG:		Should use a "last" pointer in tbuf, so that searching
 *		for capabilities alphabetically would not be a n**2/2
 *		process when large numbers of capabilities are given.
 * Note:	If we add a last pointer now we will screw up the
 *		tc capability. We really should compile stripecap.
 *
 * Essentially all the work here is scanning and decoding escapes
 * in string capabilities.  We don't use stdio because the editor
 * doesn't, and because living w/o it is not hard.
 */

/* $Log: stripecap.c,v $
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include <stripe/stripe.h>

#define MAXHOP	32	/* max number of tc= indirections */
#define BUFSIZE	512


static	char *tbuf;
static	int hopcount;	/* detect infinite loops in stripecap, init 0 */
char	*stskip();
char	*getenv();

/*
 * Get an entry for terminal name in buffer bp,
 * from the stripecap file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
stgetent(bp, name)
	char *bp, *name;
{
	register char *cp;
	register int c;
	register int i = 0, cnt = 0;
	char ibuf[BUFSIZ];
	int tf;

	tbuf = bp;

	tf = 0;
	tf = open(STRIPECAP, 0);
	if (tf < 0)
		return (-1);

	for (;;) {
		cp = bp;
		for (;;) {
			if (i == cnt) {
				cnt = read(tf, ibuf, BUFSIZ);
				if (cnt <= 0) {
					(void)close(tf);
					return (0);
				}
				i = 0;
			}
			c = ibuf[i++];
			if (c == '\n') {
				if (cp > bp && cp[-1] == '\\'){
					cp--;
					continue;
				}
				break;
			}
			if (cp >= bp+BUFSIZ) {
				(void)write(2,"Termcap entry too long\n", 23);
				break;
			} else
				*cp++ = c;
		}
		*cp = 0;

		/*
		 * The real work for the match.
		 */
		if (stnamatch(name)) {
			(void)close(tf);
			return(stnchktc());
		}
	}
}

/*
 * stnchktc: check the last entry, see if it's tc=xxx. If so,
 * recursively find xxx and append that entry (minus the names)
 * to take the place of the tc=xxx entry. This allows stripecap
 * entries to say "like an HP2621 but doesn't turn on the labels".
 * Note that this works because of the left to right scan.
 */
stnchktc()
{
	register char *p, *q;
	char tcname[16];	/* name of similar terminal */
	char tcbuf[BUFSIZ];
	char *holdtbuf = tbuf;
	int l;
	char *strcpy();

	p = tbuf + strlen(tbuf) - 2;	/* before the last colon */
	while (*--p != ':')
		if (p<tbuf) {
			(void)write(2, "Bad stripecap entry\n", 18);
			return (0);
		}
	p++;
	/* p now points to beginning of last field */
	if (p[0] != 't' || p[1] != 'c')
		return(1);
	(void)strcpy(tcname,p+3);
	q = tcname;
	while (q && *q != ':')
		q++;
	*q = 0;
	if (++hopcount > MAXHOP) {
		(void)write(2, "Infinite tc= loop\n", 18);
		return (0);
	}
	if (stgetent(tcbuf, tcname) != 1)
		return(0);
	for (q=tcbuf; *q != ':'; q++)
		;
	l = p - holdtbuf + strlen(q);
	if (l > BUFSIZ) {
		(void)write(2, "Termcap entry too long\n", 23);
		q[BUFSIZ - (p-tbuf)] = 0;
	}
	(void)strcpy(p, q+1);
	tbuf = holdtbuf;
	return(1);
}

/*
 * Tnamatch deals with name matching.  The first field of the stripecap
 * entry is a sequence of names separated by |'s, so we compare
 * against each such name.  The normal : terminator after the last
 * name (before the first field) stops us.
 */
stnamatch(np)
	char *np;
{
	register char *Np, *Bp;

	Bp = tbuf;
	if (*Bp == '#')
		return(0);
	for (;;) {
		for (Np = np; *Np && *Bp == *Np; Bp++, Np++)
			continue;
		if (*Np == 0 && (*Bp == '|' || *Bp == ':' || *Bp == 0))
			return (1);
		while (*Bp && *Bp != ':' && *Bp != '|')
			Bp++;
		if (*Bp == 0 || *Bp == ':')
			return (0);
		Bp++;
	}
}

/*
 * Skip to the next field.  Notice that this is very dumb, not
 * knowing about \: escapes or any such.  If necessary, :'s can be put
 * into the stripecap file in octal.
 */
static char *
stskip(bp)
	register char *bp;
{

	while (*bp && *bp != ':')
		bp++;
	if (*bp == ':')
		bp++;
	return (bp);
}

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *	li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
stgetnum(id)
	char *id;
{
	register int i, base;
	register char *bp = tbuf;

	for (;;) {
		bp = stskip(bp);
		if (*bp == 0)
			return (-1);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return(-1);
		if (*bp != '#')
			continue;
		bp++;
		base = 10;
		if (*bp == '0')
			base = 8;
		i = 0;
		while (isdigit(*bp))
			i *= base, i += *bp++ - '0';
		return (i);
	}
}

/* 
 * Remove an entry from the STRIPECAP file.
 * Since there is no better way to do this in UNIX, we will have to go through
 * line by line, copying the other entries to a temporary file, omitting the
 * entry in question, thus removing it from the STRIPECAP file when we copy the
 * temp file back over.
 * 						- kucera 11/10/86
 */
strment (stname)
char *stname;
{
    FILE *f;		/* pointer to the stripecap file */
    FILE *tmp;		/* pointer to the temporary copy of stripecap file */
    FILE *fopen();
    int found = 0;	/* flag if found the named entry yet */
    int end_ent;	/* flag signalling end of an entry has been reached */
    char *end;		/* index of newline character in a line read in */
    char line[BUFSIZE]; /* line read in, one at a time */
    char buf[BUFSIZE];	/* buffer for system() calls */
    char *tempfile;	/* buffer for name of temporary file */
    char *mktemp();

    if ((f = fopen(STRIPECAP, "r")) == NULL) {
	printf ("newst: Error removing entry from stripecap file\n");
 	exit (1);
    }
    tempfile = mktemp ("/tmp/newstcap.XXXXXX");
    if ((tmp = fopen(tempfile, "w")) == NULL) {
	printf ("newst: Error removing entry from stripecap file\n");
 	exit (1);
    }

    while (!found) {
	if (fgets (line, BUFSIZE, f) != NULL) { /* if not EOF */
	    if (checkname (line, stname)) { 	/* see if right entry */
		found++;			/* mark as found */
	    }
	    else 				/* not desired entry */
		fputs (line, tmp);	/* copy it to temp file */
	}
	else {				/* entry not found -> done */
     	    (void)unlink (tempfile);	/* remove temp file if there */
	    return(0);
	}
	end = index(line, '\n');
	if (*line == '\n' || (end != 0 && *(end-1) != '\\'))
	    end_ent = 1;
	else 
	    end_ent = 0;
	while (!end_ent && fgets (line, BUFSIZE, f) != NULL) {
	    end = index(line, '\n');
	    if (*line == '\n' || (end != 0 && *(end-1) != '\\'))
		end_ent = 1;
	    if (!found)				/* if not the removed entry */
		fputs (line, tmp);	/* copy line to temp file */
	}
    }
    while (fgets (line, BUFSIZE, f) != NULL)	/* for rest of stripecap file */
	fputs (line, tmp);		/* copy line to temp file */
    if (f != NULL) (void)fclose (f);
    if (tmp != NULL) (void)fclose (tmp);

    /*
     * now that the entry is removed from the copy, put the copy back
     * into the stripecap file                                     
     */
    (void)sprintf (buf, "cp %s %s", tempfile, STRIPECAP);
    if (system(buf) < 0) {
	printf ("newst: Cannot copy tempfile back into %s\n", STRIPECAP);
	exit (1);	
    }
    (void)sprintf (buf, "rm %s", tempfile);
    (void)system(buf);
    return (0);
}
    
	
/* 
 * See if this is the right entry to remove.
 */
checkname (line, name)
char *line;
char *name;
{
	register char *Lp, *Np;	    /* Lp points to the line, Np to the name */

	Lp = line;
	if (*Lp == '#')
		return(0);
	for (;;) {
		for (Np = name; *Np && *Lp == *Np; Lp++, Np++)
			continue;
		if (*Np == 0 && (*Lp == '|' || *Lp == ':' || *Lp == 0))
			return (1);
		while (*Lp && *Lp != ':' && *Lp != '|')
			Lp++;
		if (*Lp == 0 || *Lp == ':')
			return (0);
		Lp++;
	}
}


struct vtoc vtc;
struct vtctodisktab vtcdtb;

struct vtctodisktab *
tryvtoc(fs)
char *fs;

{
	int fd,index;
	char buffer[20];

	(void) sprintf (buffer, "                    ");
	(void) sprintf (buffer, "/dev/r%s", fs);
	fd = open(buffer, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: unable to open device\n", buffer);
		perror("fs");
		return((struct vtctodisktab *) -1);
	}	
	if (ioctl(fd, V_READ, (char *)&vtc) < 0) {
		fprintf(stderr, "%s: no VTOC on disk\n", buffer);
		return((struct vtctodisktab *) -1);
	}
	vtcdtb.v_disktype = vtc.v_disktype;
	vtcdtb.v_secsize = vtc.v_secsize;
	vtcdtb.v_ntracks = vtc.v_ntracks;
	vtcdtb.v_nsectors = vtc.v_nsectors;
	vtcdtb.v_ncylinders = vtc.v_ncylinders;
	vtcdtb.v_rpm = vtc.v_rpm;
	index = 0;
	while (index < 8) {
		vtcdtb.v_part[index].p_size = vtc.v_part[index].p_size;
		vtcdtb.v_part[index].p_bsize = vtc.v_part[index].p_bsize;
		vtcdtb.v_part[index].p_fsize = vtc.v_part[index].p_fsize;
		index++;
	}
	return(&vtcdtb);
}
