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
static char rcsid[] = "$Header: finger.c 2.7 1991/05/28 21:06:47 $";
#endif

/*
 * This is a finger program.  It prints out useful information about users
 * by digging it up from various system files.  It is not very portable
 * because the most useful parts of the information (the full user name,
 * office, and phone numbers) are all stored in the VAX-unused gecos field
 * of /etc/passwd, which, unfortunately, other UNIXes use for other things.
 *
 * There are three output formats, all of which give login name, teletype
 * line number, and login time.  The short output format is reminiscent
 * of finger on ITS, and gives one line of information per user containing
 * in addition to the minimum basic requirements (MBR), the full name of
 * the user, his idle time and office location and phone number.  The
 * quick style output is UNIX who-like, giving only name, teletype and
 * login time.  Finally, the long style output give the same information
 * as the short (in more legible format), the home directory and shell
 * of the user, and, if it exits, a copy of the file .plan in the users
 * home directory.  Finger may be called with or without a list of people
 * to finger -- if no list is given, all the people currently logged in
 * are fingered.
 *
 * The program is validly called by one of the following:
 *
 *	finger			{short form list of users}
 *	finger -l		{long form list of users}
 *	finger -b		{briefer long form list of users}
 *	finger -q		{quick list of users}
 *	finger -i		{quick list of users with idle times}
 *	finger namelist		{long format list of specified users}
 *	finger -s namelist	{short format list of specified users}
 *	finger -w namelist	{narrow short format list of specified users}
 *
 * where 'namelist' is a list of users login names.
 * The other options can all be given after one '-', or each can have its
 * own '-'.  The -f option disables the printing of headers for short and
 * quick outputs.  The -b option briefens long format outputs.  The -p
 * option turns off plans for long format outputs.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <utmp.h>
#include <sys/signal.h>
#include <pwd.h>
#include <stdio.h>
#include <lastlog.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define ASTERISK	'*'		/* ignore this in real name */
#define COMMA		','		/* separator in pw_gecos field */
#define COMMAND		'-'		/* command line flag char */
#define SAMENAME	'&'		/* repeat login name in real name */
#define TALKABLE	0222		/* tty is writable if 220 mode */
#define DECBUF		256		/* size of buffer for decode routine */

struct utmp user;
#define NMAX sizeof(user.ut_name)
#define LMAX sizeof(user.ut_line)
#define HMAX sizeof(user.ut_host)

#define SHORTPRINT(x)	(oldflag ? old_shortprint(x) : new_shortprint(x))
#define PERSONPRINT(x)	(oldflag ? old_personprint(x) : new_personprint(x))
#define QUICKPRINT(x)	(oldflag ? old_quickprint(x) : new_quickprint(x))
#define PHONE(x, y, z)	(oldflag ? old_phone(x, y) : new_phone(x, y, z))

struct person {			/* one for each person fingered */
	char *name;			/* name */
	char tty[LMAX+1];		/* null terminated tty line */
	char host[HMAX+1];		/* null terminated remote host name */
	long loginat;			/* time of (last) login */
	long idletime;			/* how long idle (if logged in) */
	char *realname;			/* pointer to full name */
	char *office;			/* pointer to office name */
	char *officephone;		/* pointer to office phone no. */
	char *homephone;		/* pointer to home phone no. */
	char *universe;			/* pointer to the UNIVERSE */
	struct passwd *pwd;		/* structure of /etc/passwd stuff */
	char loggedin;			/* person is logged in */
	char writable;			/* tty is writable */
	char original;			/* this is not a duplicate entry */
	char *random;			/* random gecos stuff */
	struct person *link;		/* link to next person */
};

char LASTLOG[] = "/usr/adm/lastlog";	/* last login info */
char USERLOG[] = "/etc/utmp";		/* who is logged in */
char PLAN[] = "/.plan";			/* what plan file is */
char PROJ[] = "/.project";		/* what project file */
	
int unbrief = 1;			/* -b option default */
int header = 1;				/* -f option default */
int hack = 1;				/* -h option default */
int idle = 0;				/* -i option default */
int large = 0;				/* -l option default */
int match = 1;				/* -m option default */
int plan = 1;				/* -p option default */
int unquick = 1;			/* -q option default */
int small = 0;				/* -s option default */
int wide = 1;				/* -w option default */
int oldflag = 1;			/* use old print routines */

int unshort;
int lf;					/* LASTLOG file descriptor */
struct person *person1;			/* list of people */
long tloc;				/* current time */

struct passwd *pwdcopy();
char *strcpy();
char *strcat();
char *calloc();
char *ctime();

main(argc, argv)
	int argc;
	register char **argv;
{
	FILE *fp;
	register char *s;

	/* parse command line for (optional) arguments */
	while (*++argv && **argv == COMMAND)
		for (s = *argv + 1; *s; s++)
			switch (*s) {
			case 'b':
				unbrief = 0;
				break;
			case 'f':
				header = 0;
				break;
			case 'h':
				hack = 0;
				break;
			case 'i':
				idle = 1;
				unquick = 0;
				break;
			case 'l':
				large = 1;
				break;
			case 'm':
				match = 0;
				break;
			case 'p':
				plan = 0;
				break;
			case 'q':
				unquick = 0;
				break;
			case 's':
				small = 1;
				break;
			case 'w':
				wide = 0;
				break;
			case 'n':
				oldflag = 0;
				break;
			default:
				fprintf(stderr, "Usage: finger [-bfhilmnpqsw] [login1 [login2 ...] ]\n");
				exit(1);
			}
	if (unquick || idle)
		time(&tloc);
	/*
	 * *argv == 0 means no names given
	 */
	if (*argv == 0)
		doall();
	else
		donames(argv);
	if (person1)
		print();
	exit(0);
}

doall()
{
	register struct person *p;
	register struct passwd *pw;
	int uf;
	int numnames = 0, i;

	unshort = large;
	if ((uf = open(USERLOG, 0)) < 0) {
		fprintf(stderr, "finger: error opening %s\n", USERLOG);
		exit(2);
	}
	while (read(uf, (char *)&user, sizeof user) == sizeof user) {
		if (user.ut_name[0] == 0)
			continue;
		if (person1 == 0)
			p = person1 = (struct person *) calloc(1, sizeof *p);
		else {
			p->link = (struct person *) calloc(1, sizeof *p);
			p = p->link;
		}
		p->name = (char *) malloc(strlen(user.ut_name) + 1);
		bcopy(user.ut_name, p->name, NMAX);
		p->name[NMAX] = 0;
		bcopy(user.ut_line, p->tty, LMAX);
		p->tty[LMAX] = 0;
		bcopy(user.ut_host, p->host, HMAX);
		p->host[HMAX] = 0;
		p->loginat = user.ut_time;
		p->pwd = 0;
		p->loggedin = 1;
		numnames++;
	}
	p->link = 0;
	close(uf);
	if (unquick) {
		setpwent();
		fwopen();
		i = numnames;
		while (((pw = getpwent()) != 0) && (i > 0)) {
			p = person1;
			do {
				if (p->pwd == 0) {
					if (strcmp(p->name, pw->pw_name) == 0) {
						p->pwd = pwdcopy(pw);
						decode(p);
						i--;
					}
				}
				p = p->link;
			} while (p != 0);
		}
		fwclose();
		endpwent();
	}
	if (person1 == 0) {
		printf("No one logged on\n");
		return;
	}
}

donames(argv)
	char **argv;
{
	register struct person *p;
	register struct passwd *pw;
	int uf;

	/*
	 * get names from command line and check to see if they're
	 * logged in
	 */
	unshort = !small;
	for (; *argv != 0; argv++) {
		if (!oldflag && netfinger(*argv))
			continue;
		if (person1 == 0)
			p = person1 = (struct person *) calloc(1, sizeof *p);
		else {
			p->link = (struct person *) calloc(1, sizeof *p);
			p = p->link;
		}
		p->name = *argv;
		p->loggedin = 0;
		p->original = 1;
		p->pwd = 0;
	}
	if (person1 == 0)
		return;
	p->link = 0;
	/*
	 * if we are doing it, read /etc/passwd for the useful info
	 */
	if (unquick) {
		setpwent();
		if (!match) {
/*			extern _pw_stayopen;*/

/*			_pw_stayopen = 1;*/
			for (p = person1; p != 0; p = p->link)
				if (pw = getpwnam(p->name))
					p->pwd = pwdcopy(pw);
		} else while ((pw = getpwent()) != 0) {
			for (p = person1; p != 0; p = p->link) {
				if (!p->original)
					continue;
				if (strcmp(p->name, pw->pw_name) != 0 &&
				    !matchcmp(pw->pw_gecos, pw->pw_name, p->name))
					continue;
				if (p->pwd == 0)
					p->pwd = pwdcopy(pw);
				else {
					struct person *new;
					/*
					 * handle multiple login names, insert
					 * new "duplicate" entry behind
					 */
					new = (struct person *)
						calloc(1, sizeof *new);
					new->pwd = pwdcopy(pw);
					new->name = p->name;
					new->original = 1;
					new->loggedin = 0;
					new->link = p->link;
					p->original = 0;
					p->link = new;
					p = new;
				}
			}
		}
		endpwent();
	}
	/* Now get login information */
	if ((uf = open(USERLOG, 0)) < 0) {
		fprintf(stderr, "finger: error opening %s\n", USERLOG);
		exit(2);
	}
	while (read(uf, (char *)&user, sizeof user) == sizeof user) {
		if (*user.ut_name == 0)
			continue;
		for (p = person1; p != 0; p = p->link) {
			if (p->loggedin == 2)
				continue;
			if (strncmp(p->pwd ? p->pwd->pw_name : p->name,
				    user.ut_name, NMAX) != 0)
				continue;
			if (p->loggedin == 0) {
				bcopy(user.ut_line, p->tty, LMAX);
				p->tty[LMAX] = 0;
				bcopy(user.ut_host, p->host, HMAX);
				p->host[HMAX] = 0;
				p->loginat = user.ut_time;
				p->loggedin = 1;
			} else {	/* p->loggedin == 1 */
				struct person *new;
				new = (struct person *) calloc(1, sizeof *new);
				new->name = p->name;
				bcopy(user.ut_line, new->tty, LMAX);
				new->tty[LMAX] = 0;
				bcopy(user.ut_host, new->host, HMAX);
				new->host[HMAX] = 0;
				new->loginat = user.ut_time;
				new->pwd = p->pwd;
				new->loggedin = 1;
				new->original = 0;
				new->link = p->link;
				p->loggedin = 2;
				p->link = new;
				p = new;
			}
		}
	}
	close(uf);
	if (unquick) {
		fwopen();
		for (p = person1; p != 0; p = p->link)
			decode(p);
		fwclose();
	}
}

print()
{
	register FILE *fp;
	register struct person *p;
	register char *s;
	register c;

	/*
	 * print out what we got
	 */
	if (header) {
		if (unquick) {
			if (!unshort)
				if (wide)
					printf("Login       Name              TTY Idle    When            Office\n");
				else
					printf("Login    TTY Idle    When            Office\n");
		} else {
			printf("Login      TTY            When");
			if (idle)
				if (oldflag) {
					printf( "         Idle" );
				} else {
					printf("             Idle");
				}
			putchar('\n');
		}
	}
	for (p = person1; p != 0; p = p->link) {
		if (!unquick) {
			QUICKPRINT(p);
			continue;
		}
		if (!unshort) {
			SHORTPRINT(p);
			continue;
		}
		PERSONPRINT(p);
		if (p->pwd != 0) {
			if (hack) {
				s = calloc(1, strlen(p->pwd->pw_dir) +
					sizeof PROJ);
				strcpy(s, p->pwd->pw_dir);
				strcat(s, PROJ);
				if ((fp = fopen(s, "r")) != 0) {
					printf("Project: ");
					while ((c = getc(fp)) != EOF) {
						if (c == '\n')
							break;
						if (isprint(c) || isspace(c))
							putchar(c);
						else
							break;
					}
					fclose(fp);
					putchar('\n');
				}
				free(s);
			}
			if (plan) {
				s = calloc(1, strlen(p->pwd->pw_dir) +
					sizeof PLAN);
				strcpy(s, p->pwd->pw_dir);
				strcat(s, PLAN);
				if ((fp = fopen(s, "r")) == 0)
					printf("No Plan.\n");
				else {
					printf("Plan:\n");
					while ((c = getc(fp)) != EOF)
						if (isprint(c) || isspace(c))
							putchar(c);
						else
							break;
					fclose(fp);
				}
				free(s);
			}
		}
		if (p->link != 0)
			putchar('\n');
	}
}

/*
 * Duplicate a pwd entry.
 * Note: Only the useful things (what the program currently uses) are copied.
 */
struct passwd *
pwdcopy(pfrom)
	register struct passwd *pfrom;
{
	register struct passwd *pto;

	pto = (struct passwd *) calloc(1, sizeof *pto);
#define savestr(s) strcpy(calloc(1, strlen(s) + 1), s)
	pto->pw_name = savestr(pfrom->pw_name);
	pto->pw_uid = pfrom->pw_uid;
	pto->pw_gecos = savestr(pfrom->pw_gecos);
	pto->pw_dir = savestr(pfrom->pw_dir);
	pto->pw_shell = savestr(pfrom->pw_shell);
#undef savestr
	return pto;
}

/*  print out information on quick format giving just name, tty, login time
 *  and idle time if idle is set.
 */

old_quickprint( pers )

    struct  person		*pers;
{
	printf( "%-*.*s", NMAX, NMAX, pers->name );
	printf( "  " );
	if( pers->loggedin ) {
	    if( idle ) {
		findidle( pers );
		if( pers->writable ) {
		    printf( " %-*.*s %-16.16s", LMAX, LMAX, 
			pers->tty, ctime( &pers->loginat ) );
		} else {
		    printf( "*%-*.*s %-16.16s", LMAX, LMAX, 
			pers->tty, ctime( &pers->loginat ) );
		}
		printf( "   " );
		stimeprint( &pers->idletime );
	    } else {
		printf( " %-*.*s %-16.16s", LMAX, LMAX, 
		    pers->tty, ctime( &pers->loginat ) );
	    }
	} else {
	    printf( "          Not Logged In" );
	}
	printf( "\n" );
}
/*
 * print out information on quick format giving just name, tty, login time
 * and idle time if idle is set.
 */
new_quickprint(pers)
	register struct person *pers;
{
	printf("%-*.*s  ", NMAX, NMAX, pers->name);
	if (pers->loggedin) {
		if (idle) {
			findidle(pers);
			printf("%c%-*s %-16.16s", pers->writable ? ' ' : '*',
				LMAX, pers->tty, ctime(&pers->loginat));
			ltimeprint("   ", &pers->idletime, "");
		} else
			printf(" %-*s %-16.16s", LMAX,
				pers->tty, ctime(&pers->loginat));
		putchar('\n');
	} else
		printf("          Not Logged In\n");
}

/*  print out information in short format, giving login name, full name,
 *  tty, idle time, login time, office location and phone.
 */

old_shortprint( pers )

    struct  person	*pers;

{
	struct  passwd		*pwdt = pers->pwd;
	char			buf[ 26 ];
	int			i,  len,  offset,  dialup;

	if( pwdt == (struct passwd *)0 ) {
	    printf( "%-*.*s", NMAX, NMAX,  pers->name );
	    printf( "       ???\n" );
	    return;
	}
	printf( "%-*.*s", NMAX, NMAX,  pwdt->pw_name );
	dialup = 0;
	if( wide ) {
	    if( *pers->realname )
		printf( " %-20.20s", pers->realname );
	    else
		printf( "        ???          " );
	}
	if( pers->loggedin ) {
	    if( pers->writable )
		printf( "  " );
	    else
		printf( " *" );
	} else {
	    printf( "  " );
	}
	if( *pers->tty ) {
	    strcpy( buf, pers->tty );
	    if( (buf[0] == 't')  &&  (buf[1] == 't')  &&  (buf[2] == 'y') ) {
		offset = 3;
		for( i = 0; i < 2; i++ ) {
		    buf[i] = buf[i + offset];
		}
	    }
	    if( (buf[0] == 'd')  &&  pers->loggedin ) {
		dialup = 1;
	    }
	    printf( "%-2.2s ", buf );
	} else {
	    printf( "   " );
	}
	strcpy(buf, ctime(&pers->loginat));
	if( pers->loggedin ) {
	    stimeprint( &pers->idletime );
	    offset = 7;
	    for( i = 4; i < 19; i++ ) {
		buf[i] = buf[i + offset];
	    }
	    printf( " %-9.9s ", buf );
	} else if (pers->loginat == 0)
	    printf(" < .  .  .  . >");
	else if (tloc - pers->loginat >= 180 * 24 * 60 * 60)
	    printf( " <%-6.6s, %-4.4s>", buf+4, buf+20 );
	else
	    printf(" <%-12.12s>", buf+4);
	len = strlen( pers->homephone );
	if( dialup  &&  (len > 0) ) {
	    if( len == 8 ) {
		printf( "             " );
	    } else {
		if( len == 12 ) {
		    printf( "         " );
		} else {
		    for( i = 1; i <= 21 - len; i++ ) {
			printf( " " );
		    }
		}
	    }
	    printf( "%s", pers->homephone );
	} else {
	    if( *pers->office ) {
		printf( " %-11.11s", pers->office );
		if( *pers->officephone ) {
		    printf( " %8.8s", pers->officephone );
		} else {
		    if( len == 8 )
			printf( " %8.8s", pers->homephone );
		}
	    } else {
		if( *pers->officephone ) {
		    printf( "             %8.8s", pers->officephone );
		} else {
		    if( len == 8 ) {
			printf( "             %8.8s", pers->homephone );
		    } else {
			if( len == 12 ) {
			    printf( "         %12.12s", pers->homephone );
			}
		    }
		}
	    }
	}
	printf( "\n" );
}

/*
 * print out information in short format, giving login name, full name,
 * tty, idle time, login time, office location and phone.
 */
new_shortprint(pers)
	register struct person *pers;
{
	char *p;
	char dialup;

	if (pers->pwd == 0) {
		printf("%-15s       ???\n", pers->name);
		return;
	}
	printf("%-*s", NMAX, pers->pwd->pw_name);
	dialup = 0;
	if (wide) {
		if (pers->realname)
			printf(" %-20.20s", pers->realname);
		else
			printf("        ???          ");
	}
	putchar(' ');
	if (pers->loggedin && !pers->writable)
		putchar('*');
	else
		putchar(' ');
	if (*pers->tty) {
		if (pers->tty[0] == 't' && pers->tty[1] == 't' &&
		    pers->tty[2] == 'y') {
			if (pers->tty[3] == 'd' && pers->loggedin)
				dialup = 1;
			printf("%-2.2s ", pers->tty + 3);
		} else
			printf("%-2.2s ", pers->tty);
	} else
		printf("   ");
	p = ctime(&pers->loginat);
	if (pers->loggedin) {
		stimeprint(&pers->idletime);
		printf(" %3.3s %-5.5s ", p, p + 11);
	} else if (pers->loginat == 0)
		printf(" < .  .  .  . >");
	else if (tloc - pers->loginat >= 180 * 24 * 60 * 60)
		printf(" <%-6.6s, %-4.4s>", p + 4, p + 20);
	else
		printf(" <%-12.12s>", p + 4);
	if (dialup && pers->homephone)
		printf(" %20s", pers->homephone);
	else {
		if (pers->office)
			printf(" %-11.11s", pers->office);
		else if (pers->officephone || pers->homephone)
			printf("            ");
		if (pers->officephone)
			printf(" %s", pers->officephone);
		else if (pers->homephone)
			printf(" %s", pers->homephone);
	}
	putchar('\n');
}

/* 
 *  print out a person in long format giving all possible information.
 *  directory and shell are inhibited if unbrief is clear.
 */

old_personprint( pers )

    struct  person	*pers;
{
	struct  passwd		*pwdt = pers->pwd;
	int			idleprinted;

	if( pwdt == (struct passwd *)0) {
	    printf( "Login name: %-10s", pers->name );
	    printf( "			" );
	    printf( "In real life: ???\n");
	    return;
	}
	printf( "Login name: %-10s", pwdt->pw_name );
	if( pers->loggedin ) {
	    if( pers->writable ) {
		printf( "			" );
	    } else {
		printf( "	(messages off)	" );
	    }
	} else {
	    printf( "			" );
	}
	if( *pers->realname ) {
	    printf( "In real life: %-s", pers->realname );
	}
	if( *pers->office ) {
		printf( "\nOffice: %-11.11s", pers->office );
		if( *pers->officephone )
			printf(" ,");
		else
			printf(" ");
		printf( "%-11.11s", pers->officephone );
	} else {
		printf( "\nOffice: %-11.11s  %-11.11s", pers->officephone, "" );
	}
	printf( "	Home phone: %s", pers->homephone );
	if( *pers->random )
		printf( "	%s", pers->random );
	if( unbrief ) {
	    printf( "\nDirectory: %-25s", pwdt->pw_dir );
	    if( *pwdt->pw_shell )
		printf( "	Shell: %-s", pwdt->pw_shell );
	}
	if( *pers->universe )
		printf("\nUniverse: %s\t", pers->universe);
	if( pers->loggedin ) {
	    register char *ep = ctime( &pers->loginat );
	    printf("\nOn since %15.15s on %-*.*s	", &ep[4], LMAX, LMAX, pers->tty );
	    ltimeprint("", &pers->idletime, " Idle Time");
	} else if (pers->loginat == 0)
	    printf("\nNever logged in.");
	else if (tloc - pers->loginat > 180 * 24 * 60 * 60) {
	    register char *ep = ctime( &pers->loginat );
	    printf("\nLast login %10.10s, %4.4s on %.*s",
		    ep, ep+20, LMAX, pers->tty);
	} else {
	    register char *ep = ctime( &pers->loginat );
	    printf("\nLast login %16.16s on %.*s", ep, LMAX, pers->tty );
	}
	printf( "\n" );
}

/*
 * print out a person in long format giving all possible information.
 * directory and shell are inhibited if unbrief is clear.
 */
new_personprint(pers)
	register struct person *pers;
{
	register int counter;

	if (pers->pwd == 0) {
		printf("Login name: %-10s\t\t\tIn real life: ???\n",
			pers->name);
		return;
	}
	printf("Login name: %-10s", pers->pwd->pw_name);
	if (pers->loggedin && !pers->writable)
		printf("	(messages off)	");
	else
		printf("			");
	if (pers->realname)
		printf("In real life: %s", pers->realname);
	if (pers->office) {
		printf("\nOffice: %-.11s", pers->office);
		if (pers->officephone) {
			printf(", %-18.18s", pers->officephone);
			if (pers->homephone) {
				counter = 11-strlen(pers->office);
				counter = (counter < 0 ? 0 : counter);
				printf("%*.*s Home phone: %s", counter, counter, " ", pers->homephone);
			}
		} else
			if (pers->homephone)
				printf("\t\t\tHome phone: %s", pers->homephone);
	} else if (pers->officephone) {
		printf("\nPhone: %s", pers->officephone);
		if (pers->homephone)
			printf(", %s", pers->homephone);
	} else if (pers->homephone) {
		printf("\nPhone: %s", pers->homephone);
	}
	if (unbrief) {
		printf("\nDirectory: %-25s", pers->pwd->pw_dir);
		if (*pers->pwd->pw_shell)
			printf("\tShell: %-s", pers->pwd->pw_shell);
	}
	if (pers->universe) {
		printf("\nUniverse: %s\t", pers->universe);
	}
	if (pers->loggedin) {
		register char *ep = ctime(&pers->loginat);
		if (*pers->host) {
			printf("\nOn since %15.15s on %s from %s",
				&ep[4], pers->tty, pers->host);
			ltimeprint("\n", &pers->idletime, " Idle Time");
		} else {
			printf("\nOn since %15.15s on %-*s",
				&ep[4], LMAX, pers->tty);
			ltimeprint("\t", &pers->idletime, " Idle Time");
		}
	} else if (pers->loginat == 0)
		printf("\nNever logged in.");
	else if (tloc - pers->loginat > 180 * 24 * 60 * 60) {
		register char *ep = ctime(&pers->loginat);
		printf("\nLast login %10.10s, %4.4s on %s",
			ep, ep+20, pers->tty);
		if (*pers->host)
			printf(" from %s", pers->host);
	} else {
		register char *ep = ctime(&pers->loginat);
		printf("\nLast login %16.16s on %s", ep, pers->tty);
		if (*pers->host)
			printf(" from %s", pers->host);
	}
	putchar('\n');
}

/*
 *  very hacky section of code to format phone numbers.  filled with
 *  magic constants like 4, 7 and 10.
 */

char *
old_phone( s, len )

    char		*s;
    int			len;
{
	char		fonebuf[ 15 ];
	int		i;

	switch( len ) {

	    case  3:
		fonebuf[ 0 ] = ' ';
		fonebuf[ 1 ] = 'x';
		for( i = 0; i <= 2; i++ ) {
		    fonebuf[ 2 + i ] = *s++;
		}
		fonebuf[ 5 ] = NULL;
		return (strcpy(malloc(strlen(fonebuf) + 1), fonebuf));

	    case  4:
		fonebuf[ 0 ] = ' ';
		fonebuf[ 1 ] = 'x';
		for( i = 0; i <= 3; i++ ) {
		    fonebuf[ 2 + i ] = *s++;
		}
		fonebuf[ 6 ] = NULL;
		return (strcpy(malloc(strlen(fonebuf) + 1), fonebuf));

	    case  7:
		for( i = 0; i <= 2; i++ ) {
		    fonebuf[ i ] = *s++;
		}
		fonebuf[ 3 ] = '-';
		for( i = 0; i <= 3; i++ ) {
		    fonebuf[ 4 + i ] = *s++;
		}
		fonebuf[ 8 ] = NULL;
		return (strcpy(malloc(strlen(fonebuf) + 1), fonebuf));

	    case 10:
		for( i = 0; i <= 2; i++ ) {
		    fonebuf[ i ] = *s++;
		}
		fonebuf[ 3 ] = '-';
		for( i = 0; i <= 2; i++ ) {
		    fonebuf[ 4 + i ] = *s++;
		}
		fonebuf[ 7 ] = '-';
		for( i = 0; i <= 3; i++ ) {
		    fonebuf[ 8 + i ] = *s++;
		}
		fonebuf[ 12 ] = NULL;
		return (strcpy(malloc(strlen(fonebuf) + 1), fonebuf));

	    default:
		return (strcpy(malloc(strlen(s) + 1), s));
	}
}

/*
 *  very hacky section of code to format phone numbers.  filled with
 *  magic constants like 4, 7 and 10.
 */
char *
new_phone(s, len, alldigits)
	register char *s;
	int len;
	char alldigits;
{
	char fonebuf[50];
	register char *p = fonebuf;
	register i;

	if (!alldigits)
		return (strcpy(calloc(1, len + 1), s));
	switch (len) {
	case 3:
	case 4:
		*p++ = ' ';
		*p++ = 'x';
		for (i = 0; i < len; i++) {
			*p++ = *s++;
		}
		break;
	case 5:
		*p++ = ' ';
		*p++ = 'x';
		*p++ = *s++;
		*p++ = '-';
		for (i = 0; i < 5; i++)
			*p++ = *s++;
		break;
	case 7:
		for (i = 0; i < 3; i++)
			*p++ = *s++;
		*p++ = '-';
		for (i = 0; i < 4; i++)
			*p++ = *s++;
		break;
	case 10:
		for (i = 0; i < 3; i++)
			*p++ = *s++;
		*p++ = '-';
		for (i = 0; i < 3; i++)
			*p++ = *s++;
		*p++ = '-';
		for (i = 0; i < 4; i++)
			*p++ = *s++;
		break;
	case 0:
		return 0;
	default:
		return (strcpy(calloc(1, len + 1), s));
	}
	*p++ = 0;
	return (strcpy(calloc(1, p - fonebuf), fonebuf));
}

/*
 * decode the information in the gecos field of /etc/passwd
 */
decode(pers)
	register struct person *pers;
{
	char buffer[DECBUF];
	register char *bp, *gp, *lp;
	int alldigits;
	int hasspace;
	int len;

	pers->realname = 0;
	pers->office = 0;
	pers->officephone = 0;
	pers->homephone = 0;
	pers->universe = 0;
	pers->random = 0;
	if (pers->pwd == 0)
		return;
	gp = pers->pwd->pw_gecos;
	bp = buffer;
	if (*gp == ASTERISK)
		gp++;
	while (*gp && *gp != COMMA) {			/* name */
		if (*gp == SAMENAME) {
			lp = pers->pwd->pw_name;
			if (bp >= &buffer[DECBUF]) {
				fprintf(stderr, "finger: subfield separator ',' not found. \n");
				exit(1);
			}
			if (islower(*lp))
				*bp++ = toupper(*lp++);
			while (*bp++ = *lp++) {
				if (bp >= &buffer[DECBUF]) {
					fprintf(stderr, "finger: subfield separator ',' not found. \n");
					exit(1);
				}
			}
			bp--;
			gp++;
		} else {
		/*
			if (bp >= &buffer[DECBUF]) {
				fprintf(stderr, "finger: subfield separator ',' not found. \n");
				exit(1);
			}
		*/
			*bp++ = *gp++;
		}
	}
	*bp++ = 0;
	if ((len = bp - buffer) > 1)
		pers->realname = strcpy(calloc(1, len), buffer);
	if (*gp == COMMA) {				/* office */
		gp++;
		hasspace = 0;
		bp = buffer;
		while (*gp && *gp != COMMA) {
			*bp = *gp++;
			if (*bp == ' ')
				hasspace = 1;
			if (bp < buffer + sizeof buffer - 1)
				bp++;
		}
		*bp = 0;
		len = bp - buffer;
		bp--;			/* point to last character */
		len++;
		if (len > 1)
			pers->office = strcpy(calloc(1, len), buffer);
	}
	if (*gp == COMMA) {				/* office phone */
		gp++;
		bp = buffer;
		alldigits = 1;
		while (*gp && *gp != COMMA) {
			*bp = *gp++;
			if (!isdigit(*bp))
				alldigits = 0;
			if (bp < buffer + sizeof buffer - 1)
				bp++;
		}
		*bp = 0;
		pers->officephone = PHONE(buffer, bp - buffer, alldigits);
	}
	if (*gp == COMMA) {				/* home phone */
		gp++;
		bp = buffer;
		alldigits = 1;
		while (*gp && *gp != COMMA) {
			*bp = *gp++;
			if (!isdigit(*bp))
				alldigits = 0;
			if (bp < buffer + sizeof buffer - 1)
				bp++;
		}
		*bp = 0;
		pers->homephone = PHONE(buffer, bp - buffer, alldigits);
	}
	if (*gp++ == COMMA) {
		if (strncmp(gp, "universe(", 9) == 0) {
			gp += 9;
			if ((char *)0 == (pers->universe = calloc(1, 4))) {
				fprintf(stderr, "finger: not enough memory.\n");
				exit(1);
			}
			gp[3] = '\0';
			strncpy(pers->universe, gp, 3);
			gp += 4;
		} else {
			if ((char *)0 == (pers->random =
					(char *)malloc(strlen(gp) + 1))) {
				fprintf(stderr, "finger: not enough memory.\n");
				exit(1);
			}
			strcpy(pers->random, gp);
		}
	}
	if (pers->loggedin)
		findidle(pers);
	else
		findwhen(pers);
}

/*
 * find the last log in of a user by checking the LASTLOG file.
 * the entry is indexed by the uid, so this can only be done if
 * the uid is known (which it isn't in quick mode)
 */

fwopen()
{
	if ((lf = open(LASTLOG, 0)) < 0)
		fprintf(stderr, "finger: %s open error\n", LASTLOG);
}

findwhen(pers)
	register struct person *pers;
{
	struct lastlog ll;
	int i;

	if (lf >= 0) {
		lseek(lf, (long)pers->pwd->pw_uid * sizeof ll, 0);
		if ((i = read(lf, (char *)&ll, sizeof ll)) == sizeof ll) {
			bcopy(ll.ll_line, pers->tty, LMAX);
			pers->tty[LMAX] = 0;
			bcopy(ll.ll_host, pers->host, HMAX);
			pers->host[HMAX] = 0;
			pers->loginat = ll.ll_time;
		} else {
			if (i != 0)
				fprintf(stderr, "finger: %s read error\n",
					LASTLOG);
			pers->tty[0] = 0;
			pers->host[0] = 0;
			pers->loginat = 0L;
		}
	} else {
		pers->tty[0] = 0;
		pers->host[0] = 0;
		pers->loginat = 0L;
	}
}

fwclose()
{
	if (lf >= 0)
		close(lf);
}

/*
 * find the idle time of a user by doing a stat on /dev/tty??,
 * where tty?? has been gotten from USERLOG, supposedly.
 */
findidle(pers)
	register struct person *pers;
{
	struct stat ttystatus;
	static char buffer[20] = "/dev/";
	long t;
#define TTYLEN 5

	strcpy(buffer + TTYLEN, pers->tty);
	buffer[TTYLEN+LMAX] = 0;
	if (stat(buffer, &ttystatus) < 0) {
		fprintf(stderr, "finger: Can't stat %s\n", buffer);
		exit(4);
	}
	time(&t);
	if (t < ttystatus.st_atime)
		pers->idletime = 0L;
	else
		pers->idletime = t - ttystatus.st_atime;
	pers->writable = (ttystatus.st_mode & TALKABLE) == TALKABLE;
}

/*
 * print idle time in short format; this program always prints 4 characters;
 * if the idle time is zero, it prints 4 blanks.
 */
stimeprint(dt)
	long *dt;
{
	register struct tm *delta;

	delta = gmtime(dt);
	if (delta->tm_yday == 0)
		if (delta->tm_hour == 0)
			if (delta->tm_min == 0)
				printf("    ");
			else
				if (delta->tm_min >= 10)
					printf(" %2.2d ", delta->tm_min);
				else
					printf("  %1.1d ", delta->tm_min);
		else
			if (delta->tm_hour >= 10)
				printf("%3.3d:", delta->tm_hour);
			else
				printf("%1.1d:%02.2d",
					delta->tm_hour, delta->tm_min);
	else
		printf("%3dd", delta->tm_yday);
}

/*
 * print idle time in long format with care being taken not to pluralize
 * 1 minutes or 1 hours or 1 days.
 * print "prefix" first.
 */
ltimeprint(before, dt, after)
	long *dt;
	char *before, *after;
{
	register struct tm *delta;

	delta = gmtime(dt);
	if (delta->tm_yday == 0 && delta->tm_hour == 0 && delta->tm_min == 0 &&
	    delta->tm_sec <= 10)
		return (0);
	printf("%s", before);
	if (delta->tm_yday >= 10)
		printf("%d days", delta->tm_yday);
	else if (delta->tm_yday > 0)
		printf("%d day%s %d hour%s",
			delta->tm_yday, delta->tm_yday == 1 ? "" : "s",
			delta->tm_hour, delta->tm_hour == 1 ? "" : "s");
	else
		if (delta->tm_hour >= 10)
			printf("%d hours", delta->tm_hour);
		else if (delta->tm_hour > 0)
			printf("%d hour%s %d minute%s",
				delta->tm_hour, delta->tm_hour == 1 ? "" : "s",
				delta->tm_min, delta->tm_min == 1 ? "" : "s");
		else
			if (delta->tm_min >= 10)
				printf("%2d minutes", delta->tm_min);
			else if (delta->tm_min == 0)
				printf("%2d seconds", delta->tm_sec);
			else
				printf("%d minute%s %d second%s",
					delta->tm_min,
					delta->tm_min == 1 ? "" : "s",
					delta->tm_sec,
					delta->tm_sec == 1 ? "" : "s");
	printf("%s", after);
}

matchcmp(gname, login, given)
	register char *gname;
	char *login;
	char *given;
{
	char buffer[100];
	register char *bp, *lp;
	register c;

	if (*gname == ASTERISK)
		gname++;
	lp = 0;
	bp = buffer;
	for (;;)
		switch (c = *gname++) {
		case SAMENAME:
			for (lp = login; bp < buffer + sizeof buffer
					 && (*bp++ = *lp++);)
				;
			bp--;
			break;
		case ' ':
		case COMMA:
		case '\0':
			*bp = 0;
			if (namecmp(buffer, given))
				return (1);
			if (c == COMMA || c == 0)
				return (0);
			bp = buffer;
			break;
		default:
			if (bp < buffer + sizeof buffer)
				*bp++ = c;
		}
	/*NOTREACHED*/
}

namecmp(name1, name2)
	register char *name1, *name2;
{
	register c1, c2;

	for (;;) {
		c1 = *name1++;
		if (islower(c1))
			c1 = toupper(c1);
		c2 = *name2++;
		if (islower(c2))
			c2 = toupper(c2);
		if (c1 != c2)
			break;
		if (c1 == 0)
			return (1);
	}
	if (!c1) {
		for (name2--; isdigit(*name2); name2++)
			;
		if (*name2 == 0)
			return (1);
	} else if (!c2) {
		for (name1--; isdigit(*name1); name1++)
			;
		if (*name2 == 0)
			return (1);
	}
	return (0);
}

netfinger(name)
	char *name;
{
	char *host;
	char fname[100];
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	int s;
	char *rindex();
	register FILE *f;
	register int c;
	register int lastc;

	if (name == NULL)
		return (0);
	host = rindex(name, '@');
	if (host == NULL)
		return (0);
	*host++ = 0;
	hp = gethostbyname(host);
	if (hp == NULL) {
		static struct hostent def;
		static struct in_addr defaddr;
		static char *alist[1];
		static char namebuf[128];
		int inet_addr();

		defaddr.s_addr = inet_addr(host);
		if (defaddr.s_addr == -1) {
			printf("unknown host: %s\n", host);
			return (1);
		}
		strcpy(namebuf, host);
		def.h_name = namebuf;
		def.h_addr_list = alist, def.h_addr = (char *)&defaddr;
		def.h_length = sizeof (struct in_addr);
		def.h_addrtype = AF_INET;
		def.h_aliases = 0;
		hp = &def;
	}
	sp = getservbyname("finger", "tcp");
	if (sp == 0) {
		printf("tcp/finger: unknown service\n");
		return (1);
	}
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = sp->s_port;
	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) {
		perror("socket");
		return (1);
	}
	printf("[%s]\n", hp->h_name);
	fflush(stdout);
	if (connect(s, (char *)&sin, sizeof (sin)) < 0) {
		perror("connect");
		close(s);
		return (1);
	}
	if (large) write(s, "/W ", 3);
	write(s, name, strlen(name));
	write(s, "\r\n", 2);
	f = fdopen(s, "r");
	while ((c = getc(f)) != EOF) {
		switch(c) {
		case 0210:
		case 0211:
		case 0212:
		case 0214:
			c -= 0200;
			break;
		case 0215:
			c = '\n';
			break;
		}
		lastc = c;
		if (isprint(c) || isspace(c))
			putchar(c);
		else
			break;
	}
	if (lastc != '\n')
		putchar('\n');
	(void)fclose(f);
	return (1);
}
