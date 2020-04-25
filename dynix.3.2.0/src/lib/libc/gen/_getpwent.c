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

/* $Header: _getpwent.c 1.8 90/06/12 $ */

#include <stdio.h>
#include <pwd.h>

/* not static so passwd(1) can modify it for -f */
char *__PASSWD_ = "/etc/passwd";
static char EMPTY[] = "";
static FILE *pwf = NULL;
static char line[BUFSIZ+1];
static struct passwd passwd;

#define	TRUE	1
#define	FALSE	0

_setpwent()
{
	if( pwf == NULL )
		pwf = fopen( __PASSWD_, "r" );
	else
		rewind( pwf );
}

_endpwent()
{
	if( pwf != NULL ){
		fclose( pwf );
		pwf = NULL;
	}
}

static char *
pwskip(p)
register char *p;
{
	while (*p && *p != ':' && *p != '\n')
		++p;
	if(*p == '\n')
		*p = '\0';
	else if (*p) 
		*p++ = 0;
	return(p);
}

static 
chknum(s)
char *s;
{
	register char *p = s;

	/* allow 'nobody' uid/gid -2 as a special case */
	if (p[0] == '-' && p[1] == '2' && p[2] == ':')
		return TRUE;
	/* otherwise, it must be all digits */
	while(*p >= '0' && *p <= '9')
		p++;
	if ((*p == ':') && (s != p))
		return TRUE;
	else
		return FALSE;
}

struct passwd *
_getpwent()
{
	register char *p;
	register int yp;	/* YP allows 'illegal' entries, so
				   dont check YP entries */

	if (pwf == NULL) {
		if( (pwf = fopen( __PASSWD_, "r" )) == NULL )
			return(0);
	}
	passwd.pw_quota = 0;
	passwd.pw_comment = EMPTY;

	while( p = fgets(line, BUFSIZ, pwf) ) {
		passwd.pw_name = p;
		p = pwskip(p);
		if( passwd.pw_name[0] == '\0' ) continue;	/* null field */

		yp = passwd.pw_name[0] == '+';

		passwd.pw_passwd = p;
		p = pwskip(p);

		if( !yp && !chknum(p) ) continue;	/* bad numeric */
		passwd.pw_uid = atoi(p);
		p = pwskip(p);

		if( !yp && !chknum(p) ) continue;	/* bad numeric */
		passwd.pw_gid = atoi(p);
		p = pwskip(p);

		passwd.pw_gecos = p;
		p = pwskip(p);

		passwd.pw_dir = p;
		p = pwskip(p);

		passwd.pw_shell = p;
		pwskip(p);

		return(&passwd);
	}
	return(0);
}
