/* $Header: main.c 2.27 1991/07/25 21:00:29 $ */

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

/*
 * $Log: main.c,v $
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

#ifndef lint
static char rcsid[] = "$Header: main.c 2.27 1991/07/25 21:00:29 $";
#endif

#include "crash.h"
#include "define.h"

#ifndef NOSTR
#include <string.h>
#endif
#include <signal.h>
#ifdef CROSS
#include "/usr/include/sys/stat.h"
#include "/usr/include/setjmp.h"
#else
#include <sys/stat.h>
#include <setjmp.h>
#endif
#ifdef BSD
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#define MAXKERN 600*1024
#else
#define MAXKERN 2*1024*1024

extern Dislook();
extern Dislocal();
#endif

char	*dynix = NULL;
int	kernfd;
char	*vmcore = NULL;
FILE	*vmcorefd;
char	*swap = NULL;
int	swapfd;
int	live = 0;
int	end_locore;
int	start_locore;
int	debug[MAX_DEBUG];
char *name_buf[MAX_CHECK];
char	is_compressed;

int	To = 0;
int	Eve = -1;
#ifdef BSD
jmp_buf jmp;
#else
sigjmp_buf jmp;
#endif
struct pte *Sysmap;
struct config_desc Cd_loc;
#ifndef BSD
struct	var v;
#endif

#define MAXNEST 3
static FILE *filter[MAXNEST];
static FILE *input[MAXNEST];
FILE *output;
FILE *popen();
int	nest = 0;

int tok = 0;		/* no token queue'd in token() */
int more_tok = 0;
int source = FALSE;
int script = FALSE;
int arch_type = SYSTYP_B21;	/* assume B8000 bus type */

main(argc, argv)
int	argc;
char	**argv;
{
	int	sigint(), exit(), ouch(), onpipe(), Disread();
	int	sigalrm();
	register  struct  tsw	*tp;
	register  char *c;
	struct initfuns *ip;
	struct stat sbuf;
#ifndef BSD
	int	bin_type;
#endif

	output = stdout;
	input[0] = stdin;
	signal(SIGINT, exit);

	if (strcmp(argv[1], "-d") == 0)	
		xdebug = 10, ++argv, --argc;

	if (strncmp(argv[1], "-D", 2) == 0) {
		set_debug(argv[1]);
		++argv;
		--argc;
	}

	if (strncmp(argv[1], "-X", 2) == 0) {
		debug[18] = 1;
		++argv;
		--argc;
	}

	if (strncmp(argv[1], "-I", 2) == 0) {
		++argv;
		--argc;
		source = TRUE;
		script = TRUE;
		if ((input[0] = fopen(argv[1],"r")) == NULL) {
			perror(argv[1]);
			exit(1);
		}
		++argv;
		--argc;
	}

	switch(argc) {
	default:
#ifdef BSD
		fatal("usage: crash [ dynix vmcore ]");
#else
		fatal("usage: crash [ unix vmcore ]");
#endif
	case 4:
		/* assume dmesg is include *dmesg* *unix* *vmcore* */
		++argv;
		--argc;
		/* fall through */

#ifdef BSD
	case 3:
		if (a_out(argv[2]))
			dynix = argv[2];
		else
			vmcore = argv[2];
		/* fall through */
	case 2:
		if (a_out(argv[1])) {
			if (dynix) 
				fatal("dynix specified twice!");
			dynix = argv[1];
		} else {
			if (vmcore) 
				fatal("vmcore specified twice!");
			vmcore = argv[1];
		}
		/* fall through */
	case 1:
		if (dynix == NULL)
			dynix = "/dynix";
		if (vmcore == NULL)
			vmcore = "/dev/mem";
		if (a_out(vmcore))
			fatal("vmcore is an a.out?");
		if (!a_out(dynix))
			fatal("dynix is not an a.out?");
		break;
#else
	case 3:
		bin_type = coff_bin(argv[2]);
		if (bin_type == -1) {
			printf("crash: Cannot open file '%s'\n", argv[2]);
			fflush(stderr); fflush(stdout);
			exit(1);
		} else if (bin_type == 1)
			dynix = argv[2];
		else
			vmcore = argv[2];
		/* fall through */
	case 2:
		bin_type = coff_bin(argv[1]);
		if (bin_type == -1) {
			printf("crash: Cannot open file '%s'\n", argv[1]);
			fflush(stderr); fflush(stdout);
			exit(1);
		} else if (bin_type == 1) {
			if (dynix) 
				fatal("unix specified twice!");
			dynix = argv[1];
		} else {
			if (vmcore) 
				fatal("vmcore specified twice!");
			vmcore = argv[1];
		}
		/* fall through */
	case 1:
		if (dynix == NULL)
			dynix = "/unix";
		if (vmcore == NULL)
			vmcore = "/dev/kmem";
		bin_type =  coff_bin(dynix);
		if (bin_type == -1) {
			printf("crash: cannot open file '%s'\n", dynix);
			fflush(stderr); fflush(stdout);
			exit(1);
		} else if (bin_type == 0) {
			printf("unix file '%s' is not COFF file?\n", dynix);
			fflush(stderr); fflush(stdout);
			exit(1);
		}
		break;
#endif
	}

	printf("Version %s of %s\n", Version, Date); 
	fflush(stdout);

	if ((kernfd = open(dynix, 0)) < 0)
		fatal("cannot open dynix file");
	if ((vmcorefd = fopen(vmcore, "r")) == NULL) {
		if (strcmp(vmcore, "/dev/kmem") == 0)  {
			vmcore = "/dev/openkmem";
			if ((vmcorefd = fopen(vmcore, "r")) == NULL) {
				printf("crash: cannot open vmcore file '/dev/kmem' or '%s'\n", vmcore);
				fflush(stderr); fflush(stdout);
				exit(1);
			}
		} else {
			printf("crash: cannot open vmcore file '%s'\n", vmcore);
			fflush(stderr); fflush(stdout);
			exit(1);
		}
	}

	/* Is it live or Memorex? */
	if ((strncmp(vmcore, "/dev", 4) == 0) &&
	    (strncmp(&vmcore[strlen(vmcore)-3], "mem", 3) == 0) ||
	    fstat(fileno(vmcorefd), &sbuf) == 0 && (sbuf.st_mode & S_IFMT) == S_IFCHR) {
		printf("Warning: running on a LIVE system.\n");
		if (swap == NULL)
#ifdef  BSD
			swap = "/dev/drum";
#else
			swap = "/dev/swap";
#endif
		if ((swapfd = open(swap, 0)) < 0) {
			if (strcmp(swap, "/dev/swap") == 0)  {
				swap = "/dev/openswap";
				if ((swapfd = open(swap, 0)) < 0) {
					printf("crash: cannot open swap file '/dev/swap' or '%s'\n", swap);
					fflush(stderr); fflush(stdout);
					exit(1);
				}
			} else {
				printf("crash: cannot open swap file '%s'\n", swap);
				fflush(stderr); fflush(stdout);
				exit(1);
			}
		}
		live = 1;
	} 
	if (live)
		is_compressed = 0;
	else
		core_setup();

	for(ip = initfuns; ip->i_fun ; ip++) (*ip->i_fun)();
	filter[0] = NULL;
	nest = 0;

	if (Etext == 0 || (unsigned)Etext > MAXKERN)
		printf("Warning: etext == %#x, must be HUGE.\n", Etext);


	setupsignals();

#ifdef	ns32000
	dis32000_init(0, Disread, addr_str, 1);
#endif
#ifdef	i386
#if defined(CROSS) || defined(BSD)
	dis386_init(0, Disread, addr_str, 1);
#else
	dis386_init(Disread, Dislook, 0, addr_str, Dislocal, 0, 0, 1);
#endif
#endif

	if (!live) {
		time_t t;
		readv(search("time"), &t, sizeof (time_t));
		printf("System crashed at: %.24s\n", ctime(&t));
	}


#ifdef _SEQUENT_
	(void) sigsetjmp(jmp, 1);
#else
	(void) setjmp(jmp);
#endif
	for(;;) {
		c = token();
		if (c == NULL)
			continue;
		if (debug[17])
			printf("c = token() = '%s'\n", c);
		for (tp = t; tp->t_snm; tp++) {
			if (strncmp(tp->t_snm, c, strlen(tp->t_snm)) == 0)
				break;
			if (strncmp(tp->t_nm, c, strlen(tp->t_nm)) == 0)
				break;
		}
		if ( tp->t_snm && tp->t_fun )
			(*tp->t_fun)();
		else
			printf("Huh?\n");
		tok = 0;
	}
}


setupsignals()
{
#ifdef BSD
	signal(SIGINT, sigint);
	signal(SIGSEGV, ouch);
	signal(SIGFPE, ouch);
	signal(SIGPIPE, onpipe);
	signal(SIGALRM, sigalrm);

#else
	struct sigaction act;
	sigset_t set;

	/*
	 * Signals set up with "signal()" are not reset after a signal
	 * so we use POSIX implementation.
	 */

	(void) sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = (void(*)()) sigint;
	if (sigaction(SIGINT, &act, NULL) != 0)
		fatal("main(): sigaction(SIGINT,) failed");

	act.sa_handler = (void(*)()) ouch;
	if (sigaction(SIGSEGV, &act, NULL) != 0)
		fatal("main(): sigaction(SIGSEGV,) failed");

	act.sa_handler = (void(*)()) ouch;
	if (sigaction(SIGFPE, &act, NULL) != 0)
		fatal("main(): sigaction(SIGFPE,) failed");

	act.sa_handler = (void(*)()) onpipe;
	if (sigaction(SIGPIPE, &act, NULL) != 0)
		fatal("main(): sigaction(SIGPIPE,) failed");

	act.sa_handler = (void(*)()) sigalrm;
	if (sigaction(SIGALRM, &act, NULL) != 0)
		fatal("main(): sigaction(SIGALRM,) failed");
#endif
}


sigint()
{
	tok = 0;
	more_tok = 0;
	Eve = -1;
	printf("\nInterrupt!\n");
	fflush(stdout);
	if (source == TRUE) {
		unwind();
	}
#if defined(READLINE) && defined(_SEQUENT_)
	setupsignals();	
#endif
#ifdef BSD
	longjmp(jmp, 1);
#else
	siglongjmp(jmp, 1);
#endif
}


unwind()
{
	int	i;

	for (i=1; i<nest; i++){
		if (xdebug)
			fprintf(stderr,"uwind form %d\n",nest);
		if (input[nest])
			fclose(input[nest]);
		input[nest] = NULL;
		if (filter[nest])
			pclose(filter[nest]);
		input[nest] = NULL;
	};
	if (filter[0])
		pclose(filter[0]);
	filter[0] = NULL;
	output = stdout;
	nest = 0;
	if (script)
		source = TRUE;
	else
		source = FALSE;
}


sigalrm()
{
	To = 1;
#if defined(READLINE) && defined(_SEQUENT_)
	setupsignals();	
#endif
}

ouch(sig)
	int sig;
{
	int fltaddr;

#ifdef _SEQUENT_
	sigrelse(SIGSEGV);
#endif
#if defined(READLINE) && defined(_SEQUENT_)
	setupsignals();	
#endif
	tok = 0;
	more_tok = 0;
	fflush(stdout);
	fflush(stderr);
	fflush(stdin);
#ifdef BSD
	printf("Ouch!, take off hoser.\n");
	fflush(stdout);
	longjmp(jmp, 1);
#else
	sigcontext(SF_SEGVCODE,&fltaddr);
	printf("Ouch!, take off hoser. 0x%x\n",fltaddr);
	fflush(stdout);
	siglongjmp(jmp, 1);
#endif
}

onpipe()
{
	tok = 0;
	more_tok = 0;
#ifdef BSD
	longjmp(jmp, 1);
#else
	siglongjmp(jmp, 1);
#endif
}

char pline[512];

char *
token()
{
	extern	char	*strchr();
	extern	char	*strncpy();
	extern	char	*nextline();
	char  *cp;
	static char line[BUFSIZ];
	static char askline[BUFSIZ];
	static char *p;
	static int  more_tok;
	char 	    *pp;
	int	    n;
	char 	    *tmp;
	int	    paren;

	if (tok == 0) {
		tok = more_tok;
	}
	paren = 0;
	while(tok == 0) {
		if (filter[nest] != NULL) {
			if (xdebug)
				fprintf(stderr,"filer[%d] closes\n",nest);
			(void) pclose(filter[nest]);
			filter[nest] = NULL;
			if (nest && filter[nest-1])
				output = filter[nest-1];
			else
				output = stdout;
		}
		if ((Eve >= 0) && (To==0)) {
			p = line;
			if (Eve) {
#ifdef BSD
				sigblock(1<<SIGALRM);
				alarm(Eve);
				sigpause(0);
#else
				sighold(SIGALRM);
				alarm(Eve);
				sigpause(SIGALRM);
#endif
			}
			alarm(0);
			(void) strcpy(p, pline);
		} else {
			if((p = nextline(line)) == NULL)
				continue;
		}
		To = 0;
		if (xdebug && source == TRUE)
			fprintf(stderr, "+ %s", p);
		if (strncmp(p, "!!", 2) == 0) {
			if (!pline[0]) {
				printf("no previous command\n");
				continue;
			}
			if (strlen(p)+strlen(pline) >= sizeof line) {
				printf("line too long\n");
				continue;
			}
			strcpy(pline+strlen(pline)-1, p+2);
			strcpy(p, pline);
			fprintf(stderr, "%s", p);
		} else 
			if (p[0] != '\n')
				strcpy(pline, p);	/* salt away current command */
		
		if (*p == '!') {		/* shell escape */
			system(&line[1]);
			continue;
		}
		if (*p == '\033') { /* local esacpe sequence */
			continue;
		}
		/*
		 * Check for "|" into a shell command line
		 */
		pp = p;
		while ((pp = strchr(pp, '|')) != NULL) {
			if (!isspace(pp[-1])) {
				++pp; continue;
			}
			*pp = 0;
			if (xdebug)
				fprintf(stderr,"filt[%d]=%s\n",nest,pp+1);
			filter[nest] = popen(pp+1, "w");
			if (filter[nest] == NULL) {
				printf("crash: can't pipe to %s\n", pp+1);
				fflush(stdout);
				tok = 0;
			} else {
				output = filter[nest];
			}
		}
		if (*p == '"') {
			pp = p;
			while (*p != '\n') {
				++p;
				if (*p == '"')
					break;
			}
			if ((*p == '\n') || (*(p+1) == '\n')) {
				*p++ = '\0';
				printf("%s\n",pp+1);
			} else {
				*p++ = '\0';
				printf("%s\" ",pp);
			}
		}
		if (*p == '.') {	/* source file */
			p++;
			if (!isspace(*p)) {
				fprintf(stderr,"'.' command must be followed by white space\n");
				continue;
			}
			while (*p != '\n' && isspace(*p)) ++p;
			if (*p == '\n') {
				fprintf(stderr, "source what?\n");
				continue;
			}
			pp = p;
			while (*p != '\n' && !isspace(*p)) ++p;
			if (*p != '\n') {
				*p = '\0';
				p++;
			} else
				*p = '\0';
			if (nest >= MAXNEST) {
				fprintf(stderr, "source nesting too deep\n");
				unwind();
				continue;
			}
			source = TRUE;
			nest++;
			if (xdebug)
				fprintf(stderr,"open nest=%d = %s\n",nest,pp);
			if ((input[nest] = fopen(pp, "r")) == NULL) {
				fprintf(stderr, "can't source %s\n", pp);
				unwind();
			}
			continue;
		} 
		tok = 1;
		more_tok = 0;
	}
	cp = NULL;
	for(;;) {
		switch(*p) {
		case '\n':
			*p = '\0';
			more_tok = 0;
		case '\0':
			if (cp == NULL)
				tok = 0;
			if (more_tok)
				p++;
			break;

		case '(':
			paren++;
			if (cp == NULL)
				cp = p;
			++p;
			continue;
		case ')':
			paren--;
			if (cp == NULL)
				cp = p;
			++p;
			continue;

		case '@':
			pp = p+1;
			if (cp != NULL) {
				n = p-cp;
				cp = strncpy(askline, cp, n); 
			} else 
				n = 0;
			p = &askline[n];
			(void)fgets(p, 512-n, stdin);
			strcpy(strchr(p,'\n'), pp);
			strcat(p, "\n");
			continue;

		case ';':
			*p = '\0';
			more_tok = 1;
			break;

		case ' ':
		case '\t':
			if (paren == 0) {
				*p++ = '\0';
				if (cp == NULL)
					continue;
				break;
			}
			/* fall through */
		default:
			if (cp == NULL)
				cp = p;
			++p;
			continue;
		}
		return(cp);
	}
}

fatal(str)
{
	printf("crash: %s\n", str);
	fflush(stderr); fflush(stdout);
	exit(1);
}

int paniced;
char *panicstr;
char *version;

heuristics()
{
	char buf1[512];
	char buf2[512];
	register i,j;
	register char *p;
	char c;
	int	loc;
	unsigned char a_type;

	/*
	 * Locate kernel version in kernel
	 */
	bzero(buf1, sizeof buf1);
	bzero(buf2, sizeof buf2);
	version = (char *)search("version");
	if (version == NULL)
		printf("Warning: No 'version' symbol in symbol table!\n");
	else {
		loc = (long) version - data_start + data_offset;
		lseek(kernfd, loc, 0);
		if (read(kernfd, buf1, sizeof (buf1)-1) != sizeof (buf1)-1)
#ifdef BSD
			printf("Warning: read error on dynix version string\n");
#else
			printf("Warning: read error on unix version string\n");
#endif
		if (is_compressed){
			if (core_read((long) version, buf2, sizeof (buf2)-1) !=
				sizeof (buf2)-1)
				printf("Warning: read error on vmcore version string\n");
		} else {
			lseek(fileno(vmcorefd), (long) version, 0);
			if (read(fileno(vmcorefd), buf2, sizeof (buf2)-1) != 
				sizeof (buf2)-1) 
				printf("Warning: read error on vmcore version string\n");
		}
		buf1[strlen(buf1)-1] = '\0';
		buf2[strlen(buf2)-1] = '\0';
		if (buf1[0] == '\0' || buf2[0] == '\0')
			printf("Warning: version string NULL, memory probably zero'ed or dump taken incorrectly\n");
		else if (strcmp(buf1, buf2) != 0) {
			printf("Warning: version string mismatch:\n");
#ifdef BSD
			printf("\t dynix: %s\n", buf1);
#else
			printf("\t  unix: %s\n", buf1);
#endif
			printf("\tvmcore: %s\n", buf2);
		} else
			printf("Version: %s\n", buf1);
	}

	/*
	 * Locate panic string in kernel
	 */
	bzero(buf1, sizeof buf1);
	if (!live && readv(search("panicstr"), &panicstr, sizeof panicstr) == sizeof panicstr) {
		if (panicstr == NULL)
			printf("Warning: panicstr is NULL\n");
		else {
			paniced = 1;
			for (j=i=0; j < sizeof buf1; i++,j += strlen(p)) {
				if (readv(panicstr+i, &c, 1) != 1 || c == 0)
					break;
				strcat(&buf1[j], p = nice_char(c));
			}
			if (j < sizeof buf1)
				printf("Panicstr: %s\n", buf1);
		}
	}

	/*
	 * Determine archiceture type.
	 * This tells us if SB8000 bus or EISA bus.
	 * That information is used in "misc.c" to read core info.
	 */

	if ( (search("arch_type") == NULL) ||
	     (readv(search("arch_type"), &a_type, sizeof a_type) == -1) ) {
#ifndef BSD
		printf("Cannot determne architecure type.\n");
		printf("Assuming SB8000 bus type.\n");
#else
#ifdef i386
		arch_type = SYSTYP_S27;
		printf("(SYSTYP_S27)\n");
#else
		printf("(SYSTYP_B21)\n");
#endif
#endif
	} else {
		arch_type = (int) a_type;
		printf("Architecture type (arch_type) = %d ", arch_type);
		switch (arch_type) {
		case SYSTYP_B8:
			printf("(SYSTYP_B8)\n");
			break;
		case SYSTYP_B21:
			printf("(SYSTYP_B21)\n");
			break;
		case SYSTYP_S27:
			printf("(SYSTYP_S27)\n");
			break;
		case SYSTYP_S81:
			printf("(SYSTYP_S81)\n");
			break;
#ifdef SYSTYP_S16
		case SYSTYP_S16:
			printf("(SYSTYP_S16)\n");
			break;
#endif
#ifdef SYSTYP_S4
		case SYSTYP_S4:
			printf("(SYSTYP_S4)\n");
			break;
#endif
#ifdef SYSTYP_S3
		case SYSTYP_S3:
			printf("(SYSTYP_S3)\n");
			break;
#endif
		default:
			printf("\n");
			break;
		}
	}

	get_locore();
}


/* Suppport for extended debugging. */

set_debug(p)
	char *p;
{
	char buf[512];

	p += 2;			/* skip "-D" */

	if (*p && *p >= '0' && *p <= '9') {
		scan_debug(p);
		if (debug[13])
			add_names();
		return;
	}

loop:
	printf("\ndebug on? (y/n/h/#) [n]: ");
	(void) gets(buf);
	if (buf[0] == 'n' || buf[0] == 'N' || buf[0] == '\0') {
		return;
	} else if (buf[0] == 'h'|| buf[0] == 'H') {
		show_debug_list();
		goto loop;
	} else if (buf[0] == 'y' || buf[0] == 'Y') {
		buf[0] = '\0';
		printf("Enter debug list: ");
		(void) gets(buf);
		if (buf[0] > '0' && buf[0] < '9') {
			scan_debug(buf);
			if (debug[13])
				add_names();
			return;
		} else
			goto loop;
	} else if (buf[0] >= '0' && buf[0] <= '9') {
		scan_debug(buf);
		if (debug[13])
			add_names();
		return;
	} else
		goto loop;
}

/*
 * Lifted from existing code, this parses a string for numbers.
 * Strings may contain blank or comma separated numbers, and
 * an inclusive list to be indicated with a dash.
 *
 *    0 1 2 3 5 6
 *    0-3 5 6
 *    0-3,5,6
 */

scan_debug(s)
	char *s;
{
	register char *p;
	char *q;
	register int n;
	register int i;
	int f;

	p = s;
	for (;;) {
		n = f = 0;
		/* check for '0' to '9' */
		if (*p < '0' || *p > '9') {
			printf("Don't understand: %s\n", s);
			printf("                  ");
			q = s;
			while (*q != *p) {
				putchar(' ');
				q++;
			}
			printf("^\n");
			return;
		}
		/* decode number */
		while (*p >= '0' && *p <= '9') {
			n = (n * 10) + ((int) (*p - '0')) ;
			p++;
		}
		/* make sure legal range */
		if (n >= MAX_DEBUG) {
			printf("Debug range is 0 to %d\n", MAX_DEBUG - 1);
			return;
		}
		/* terminate if done */
		if (!*p) {
			debug[n] = ( debug[n] ? 0 : 1 );
			return;
		}
		/* loop if separator */
		if (*p == ' ' || *p == ',') {
			debug[n] = ( debug[n] ? 0 : 1 );
			p++;
			while(*p == ' ')
				p++;
			continue;
		}
		/* must be '-' for inclusive range */
		if (*p != '-') {
			printf("Don't understand: %s\n",s);
			printf("                  ");
			q = s;
			while (*q != *p) {
				putchar(' ');
				q++;
			}
			printf("^\n");
			return;
		}
		f = n;
		n = 0;
		p++;
		/* check for '0' to '9' */
		if (*p < '0' || *p > '9') {
			printf("Expeced number: %s\n", s);
			printf("                ");
			q = s;
			while (*q != *p) {
				putchar(' ');
				q++;
			}
			printf("^\n");
			return;
		}
		/* decode number */
		while (*p >= '0' && *p <= '9') {
			n = (n * 10) + ((int) (*p - '0')) ;
			p++;
		}
		/* make sure legal range */
		if (n >= MAX_DEBUG) {
			printf("Debug range is 0 to %d\n", MAX_DEBUG - 1);
			return;
		}
		for (i = f; i <= n; i++)
			debug[i] = ( debug[i] ? 0 : 1 );
		/* see if done */
		if (!*p)
			return;
	}
}

char *dbg_msg[] = {
"0  - informative messages",
"1  - process() - unsorted ptr list of globals - by name",
"2  - process() - unsorted ptr list of dbg",
"3  - process() - sorted ptr list of globals - by name",
"4  - process() - sorted ptr list of dbg",
"5  - cmpsymbyval() - show value comparisons",
"6  - cmpsymbynam() - show name comparisons",
"7  - cmpsdbbynam() - show sdb name comparisons",
"8  - sym_add() - show name/val added",
"9  - sym_add_dbg() - show sdb name/val added",
"10 - mallocs - show mallocs and reallocs",
"11 - look_dupname() - show searches",
"12 - look_dupsdb() - show searches",
"13 - search for names read from symbol table",
"14 - display duplicate symbol names",
"15 - trace expression parsing",
"16 - trace proc table",
"17 - trace token parsing",
"18 - display warnings for failed symbol search",
"19 - trace stack trace code (printstacktrace)\n",
0
};

show_debug_list()
{
	char **p;
	int i = 0;

	p = dbg_msg;
	while (*p) {
		if (debug[i++])
			printf("+ ");
		else
			printf("  ");
		printf("%s\n", *p);
		p++;
	}
}

add_names()
{
	int i;
	char *save_name();
	char buf[512];

	printf("Enter symbol names for debug, return only to stop.\n");
	for (i=0 ;i < MAX_CHECK; i++)
		name_buf[i] = "";

	for (i=0 ; i < MAX_CHECK; i++) {
		printf("Name %d: ", i);
		gets(buf);
		if (buf[0] == '\0')
			return;
		name_buf[i] = save_name(buf);
	}

}


ttydrain()
{ 
#ifndef BSD
	tcdrain(1);
#else
	struct sgttyb stty;
	struct itimerval it;
	int	data;
	int	i;

	fflush(stdout);
	fflush(stderr);
	fflush(stdin);

	if ((ioctl(1,TIOCCBRK, 0) == -1) && (errno == ENOTTY)) {
		ioctl(1,TIOCSTOP, 0);		/* wakes up master psuedo tty */
		ioctl(1,TIOCSTART, 0);		

		it.it_interval.tv_sec = 0;
		it.it_interval.tv_usec = 0;
		it.it_value.tv_sec = 0;
		it.it_value.tv_usec = 500000;

		do {
			sigblock(1<<SIGALRM);
			setitimer(ITIMER_REAL, &it, 0);
			sigpause(0);
			To = 0;
			ioctl(0,TIOCOUTQ,&data);
		} while (data);
		for (i = 0; i<200; i++)
			write(1, "\r" ,1);
	}
#endif
}


#ifdef READLINE
char *
nextline(line)
	char *line;
{
	extern char *readline();
	char *tmp;
	char *p;

	p = line;
	if ((source == FALSE) && !filter[nest]) {
		ttydrain();
		tmp = readline("> ");
		if (!tmp)
			return("quit");
		if (*tmp)
			add_history(tmp);
		strcpy(p, tmp);
		free(tmp);
		strcat(p, "\n");
#ifdef _SEQUENT_
		setupsignals();	
#endif
	} else if(fgets(p, BUFSIZ, input[nest]) == NULL) {
		if (source == TRUE) {
			fclose(input[nest]);
			if (filter[nest]) {
				pclose(filter[nest]);
				filter[nest] = NULL;
				if (nest && filter[nest-1])
					output = filter[nest-1];
				else
					output = stdout;
			}
			if (--nest == 0) {
				source = FALSE;
			}
			if (xdebug)
				fprintf(stderr, "nest=%d\n",nest);
			return(NULL);
		}
		return("quit\n");
	}
	if (script) {
		printf("> %s",p);
		sleep(5);
	}
	return (p);
}
#else
char *
nextline(line)
	char *line;
{
	char *p;

	p = line;
	if ((source == FALSE) && !filter[nest]) {
		printf("> ");
		fflush(stdout);
	}
	if(fgets(p, BUFSIZ, input[nest]) == NULL) {
		if (source == TRUE) {
			fclose(input[nest]);
			if (filter[nest]) {
				pclose(filter[nest]);
				filter[nest] = NULL;
				if (nest && filter[nest-1])
					output = filter[nest-1];
				else
					output = stdout;
			}
			if (--nest == 0) {
				source = FALSE;
			}
			if (xdebug)
				fprintf(stderr, "nest=%d\n",nest);
			return(NULL);
		}
		return("quit\n");
	}
	return(p);
}
#endif
