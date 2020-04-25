#ifndef lint
static char *RCSid =
    "$Header: /usr/src/dynix.3.2.0/src/5lib/libc/port/sys/RCS/uname.c,v 1.4 1993/03/09 08:55:22 bruce Exp $";
#endif

/*
	uname -- system call emulation for 4.2BSD

	from: last edit:	05-Mar-1984	D A Gwyn
*/

#include	<errno.h>
#include	<string.h>
#include	<sys/utsname.h>

#ifndef SYSNAME
SYSNAME must be defined (via make or cc)!
E.g. -DSYSNAME=\"DYNIX\"
#endif

#ifndef RELEASE
RELEASE must be defined (via make or cc)!
E.g. -DRELEASE=\"3.0\"
#endif

#ifndef VERSION
VERSION must be defined (via make or cc)!
E.g. -DVERSION=\"12\"
#endif

#define NULL	0

extern int	_gethostname();

int
uname( name )
register struct utsname *name;	/* where to put results */
{
	register char		*np;	/* -> array being cleared */

	if ( name == NULL ) {
		errno = EFAULT;
		return -1;
	}

	for (np = name->nodename; np < &name->nodename[sizeof name->nodename];)
		*np++ = '\0';		/* for cleanliness */

	if ( _gethostname( name->nodename, sizeof name->nodename ) != 0)
		(void)strcpy( name->nodename, "unknown" );

	(void)strncpy( name->sysname, SYSNAME, sizeof name->sysname-1);

	(void)strncpy( name->release, RELEASE, sizeof name->release-1);

	(void)strncpy( name->version, VERSION, sizeof name->version-1);

	(void)strncpy( name->machine,
#ifdef	i386
			"i386",
#else
#ifdef	ns32000
			"ns32000",
#else
#ifdef	interdata
		       "interdata",
#else
#ifdef	pdp11
		       "pdp11",
#else
#ifdef	sun
		       "sun",
#else
#ifdef	u370
		       "u370",
#else
#ifdef	u3b
		       "u3b",
#else
#ifdef	vax
		       "vax",
#else
#ifdef	m68k
		       "m68k",
#else
		       "unknown",
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
		       sizeof name->machine-1);

	return 0;
}
