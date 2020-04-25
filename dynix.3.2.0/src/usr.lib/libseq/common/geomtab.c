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

/*
 * "$Header: geomtab.c 1.3 90/02/15 $"
 */

#include <sys/types.h>
#include <sys/param.h>
#include <zdc/zdc.h>
#include <diskinfo.h>
#include <stdio.h>
#ifdef SYSV
#include <dirent.h>
#else
#include <sys/dir.h>
#define strrchr rindex
#endif

#ifdef NOTDEF
static	char *ggetstr();
#endif

struct geomtab *
getgeombyname(name)
	char *name;
{
	extern char *strcpy();
	static struct geomtab geom;
	static char localbuf[128];
	char *cp = localbuf;
	register struct	geomtab *gp = &geom;
	char buf[BUFSIZ];

	if (ggetent(buf, name) <= 0)
		return ((struct geomtab *)0);
	gp->g_name = cp;
	(void) strcpy(cp, name);
	cp += strlen(name) + 1;
	gp->g_secsize = ggetnum("se");
	if (gp->g_secsize < 0)
		gp->g_secsize = 512;
	gp->g_ntracks = ggetnum("nt");
	gp->g_nsectors = ggetnum("ns");
	gp->g_ncylinders = ggetnum("nc");
	gp->g_rpm = ggetnum("rm");
	if (gp->g_rpm < 0)
		gp->g_rpm = 3600;
	gp->g_capacity = ggetnum("dc");
	gp->g_nseccyl = ggetnum("cy");
	gp->g_mincap = ggetnum("xc");
	return (gp);
}

#include <ctype.h>

static	char *tbuf;
static	char *gskip();
#ifdef NOTDEF
static	char *gdecode();
#endif

/*
 * Get an entry for disk name in buffer bp,
 * from the geomcap file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
static
ggetent(bp, name)
	char *bp, *name;
{
	extern char *strrchr();
	char gbuf[MAXPATHLEN];
	int tf;
	char	*p;
#ifdef SYSV
struct  dirent  *dir;
#else
struct  direct  *dir;
#endif
	DIR	*infodir;

	tbuf = bp;
	/*
	 * Try for a matching file first.
	 */

	(void) sprintf(gbuf, "%s/%s%s", INFODIR, name, GEOMSUFF);
	if ((tf = open(gbuf, 0)) >= 0) {
		if (ggetfile(name, tf)) {
			close(tf);
			return (1);
		}
		close(tf);
	}

	/*
	 * failed so try *.geom
	 */
	if ((infodir = opendir(INFODIR)) == NULL) {
		(void) write(2, "cannot open ", 12);
		(void) write(2, INFODIR, sizeof(INFODIR) );
		(void) write(2, "\n", 1);
		return (-1);
	}
	while ((dir=readdir(infodir)) != NULL) {
		if ((p=strrchr(dir->d_name, '.')) == NULL)
			continue;
		if ((strcmp(p, GEOMSUFF)) == 0) {
			(void) sprintf(gbuf, "%s/%s", INFODIR, dir->d_name);
			if ((tf = open(gbuf, 0)) >= 0) {
				if (ggetfile(name, tf)) {
					close(tf);
					closedir(infodir);
					return (1);
				}
				close(tf);
			}
		}
	}
	closedir(infodir);
	return (-1);
}

/*
 * looks in a geometry files for a matching name.
 */
static
ggetfile(name, tf)
	char *name;
	int  tf;
{
	register char *cp;
	register int c;
	register int i = 0, cnt = 0;
	char ibuf[BUFSIZ];

	for (;;) {
		cp = tbuf;
		for (;;) {
			if (i == cnt) {
				if ((cnt = read(tf, ibuf, BUFSIZ)) <= 0) {
					return (0);
				}
				i = 0;
			}
			c = ibuf[i++];
			if (c == '\n') {
				if (cp > tbuf && cp[-1] == '\\'){
					cp--;
					continue;
				}
				break;
			}
			if (cp >= tbuf+BUFSIZ) {
				write(2,"Disktab entry too long\n", 23);
				break;
			} else
				*cp++ = c;
		}
		*cp = 0;

		/*
		 * The real work for the match.
		 */
		if (gnamatch(name)) {
			return (1);
		}
	}
}

/*
 * Gnamatch deals with name matching.  The first field of the geomtab
 * entry is a sequence of names separated by |'s, so we compare
 * against each such name.  The normal : terminator after the last
 * name (before the first field) stops us.
 */
static
gnamatch(np)
	char *np;
{
	register char *Np, *Bp;

	Bp = tbuf;
	if (*Bp == '#')
		return (0);
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
 * into the geomtab file in octal.
 */
static char *
gskip(bp)
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
static
ggetnum(id)
	char *id;
{
	register int i, base;
	register char *bp = tbuf;

	for (;;) {
		bp = gskip(bp);
		if (*bp == 0)
			return (-1);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return (-1);
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

#ifdef NOTDEF
/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a : or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given.
 */
static
ggetflag(id)
	char *id;
{
	register char *bp = tbuf;

	for (;;) {
		bp = gskip(bp);
		if (!*bp)
			return (0);
		if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1]) {
			if (!*bp || *bp == ':')
				return (1);
			else if (*bp == '@')
				return (0);
		}
	}
}
#endif

#ifdef NOTDEF
/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
static char *
ggetstr(id, area)
	char *id, **area;
{
	register char *bp = tbuf;

	for (;;) {
		bp = gskip(bp);
		if (!*bp)
			return (0);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return (0);
		if (*bp != '=')
			continue;
		bp++;
		return (gdecode(bp, area));
	}
}
#endif

#ifdef NOTDEF
/*
 * Gdecode does the grung work to decode the
 * string capability escapes.
 */
static char *
gdecode(str, area)
	register char *str;
	char **area;
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = *area;
	while ((c = *str++) && c != ':') {
		switch (c) {

		case '^':
			c = *str++ & 037;
			break;

		case '\\':
			dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
			c = *str++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (isdigit(c)) {
				c -= '0', i = 2;
				do
					c <<= 3, c |= *str++ - '0';
				while (--i && isdigit(*str));
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	str = *area;
	*area = cp;
	return (str);
}
#endif
