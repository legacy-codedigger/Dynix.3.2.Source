/*	@(#)msg.c	1.6	*/
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */


#include	"defs.h"
#include	"sym.h"

/*
 * error messages
 */
char	badopt[]	= "bad option(s)";
char	mailmsg[]	= "you have mail\n";
char	nospace[]	= "no space";
char	nostack[]	= "no stack space";
char	synmsg[]	= "syntax error";

char	badnum[]	= "bad number";
char	badparam[]	= "parameter null or not set";
char	unset[]		= "parameter not set";
char	badsub[]	= "bad substitution";
char	badcreate[]	= "cannot create";
char	nofork[]	= "fork failed - too many processes";
char	noswap[]	= "cannot fork: no swap space";
char	restricted[]	= "restricted";
char	piperr[]	= "cannot make pipe";
char	badopen[]	= "cannot open";
char	coredump[]	= " - core dumped";
char	arglist[]	= "arg list too long";
char	txtbsy[]	= "text busy";
char	toobig[]	= "too big";
char	badexec[]	= "cannot execute";
char	notfound[]	= "not found";
char	badfile[]	= "bad file number";
char	badshift[]	= "cannot shift";
char	baddir[]	= "bad directory";
char	badtrap[]	= "bad trap";
char	wtfailed[]	= "is read only";
char	notid[]		= "is not an identifier";
char 	badulimit[]	= "Bad ulimit";
char	badreturn[] = "cannot return when not in function";
char	badexport[] = "cannot export functions";
char	badunset[] 	= "cannot unset";
char	nohome[]	= "no home directory";
char 	badperm[]	= "execute permission denied";
char	longpwd[]	= "sh error: pwd too long";
/*
 * messages for 'builtin' functions
 */
char	btest[]		= "test";
char	badop[]		= "unknown operator ";
/*
 * built in names
 */
char	pathname[]	= "PATH";
char	cdpname[]	= "CDPATH";
char	homename[]	= "HOME";
char	mailname[]	= "MAIL";
char	ifsname[]	= "IFS";
char	ps1name[]	= "PS1";
char	ps2name[]	= "PS2";
char	mchkname[]	= "MAILCHECK";
char	acctname[]  	= "SHACCT";
char	mailpname[]	= "MAILPATH";

/*
 * string constants
 */
char	nullstr[]	= "";
char	sptbnl[]	= " \t\n";
char	defpath[]	= ":/bin:/usr/bin";
char	colon[]		= ": ";
char	minus[]		= "-";
char	endoffile[]	= "end of file";
char	unexpected[] 	= " unexpected";
char	atline[]	= " at line ";
char	devnull[]	= "/dev/null";
char	execpmsg[]	= "+ ";
char	readmsg[]	= "> ";
char	stdprompt[]	= "$ ";
char	supprompt[]	= "# ";
char	profile[]	= ".profile";
char	sysprofile[]	= "/etc/profile";

/*
 * tables
 */

struct sysnod reserved[] =
{
	{ "case",	CASYM	},
	{ "do",		DOSYM	},
	{ "done",	ODSYM	},
	{ "elif",	EFSYM	},
	{ "else",	ELSYM	},
	{ "esac",	ESSYM	},
	{ "fi",		FISYM	},
	{ "for",	FORSYM	},
	{ "if",		IFSYM	},
	{ "in",		INSYM	},
	{ "then",	THSYM	},
	{ "until",	UNSYM	},
	{ "while",	WHSYM	},
	{ "{",		BRSYM	},
	{ "}",		KTSYM	}
};

int no_reserved = 15;

char	*sysmsg[NSIG] =
{
	0,				/* 0 */
	"Hangup",			/* 1 */
	0,	/* Interrupt */		/* 2 */
	"Quit",				/* 3 */
	"Illegal instruction",		/* 4 */
	"Trace/BPT trap",		/* 5 */
	"abort",			/* 6 */
	"EMT trap",			/* 7 */
	"Floating exception",		/* 8 */
	"Killed",			/* 9 */
	"Bus error",			/* 10 */
	"Memory fault",			/* 11 */
	"Bad system call",		/* 12 */
	0,	/* Broken pipe */	/* 13 */
	"Alarm call",			/* 14 */
	"Terminated",			/* 15 */
	"urgent I/O",			/* 16 */
	"Stop Signal (SIGSTOP)",	/* 17 */
	"Stop Signal (SIGTSTP)",	/* 18 */
	"Continue Signal",		/* 19 */
	"Child death",			/* 20 */
	"Stop signal (SIGTTIN)",	/* 21 */
	"Stop signal (SIGTTOU)"	,	/* 22 */
	"I/O possible",			/* 23 */
	"CPU time limit exceeded",	/* 24 */
	"File size limit exceeded",	/* 25 */
	"Virtual alarm",		/* 26 */
	"Profiling alarm",		/* 27 */
	"Window changed size",		/* 28 */
	"Signal 29",			/* 29 */
	"User defined signal 1",	/* 30 */
	"User defined signal 2",	/* 31 */
};

char	export[] = "export";
char	duperr[] = "cannot dup";
char	readonly[] = "readonly";


struct sysnod commands[] =
{
	{ ".",		SYSDOT	},
	{ ":",		SYSNULL	},

#ifndef RES
	{ "[",		SYSTST },
#endif

	{ "break",	SYSBREAK },
	{ "cd",		SYSCD	},
	{ "continue",	SYSCONT	},
	{ "echo",	SYSECHO },
	{ "eval",	SYSEVAL	},
	{ "exec",	SYSEXEC	},
	{ "exit",	SYSEXIT	},
	{ "export",	SYSXPORT },
	{ "hash",	SYSHASH	},

#ifdef RES
	{ "login",	SYSLOGIN },
	{ "newgrp",	SYSLOGIN },
#else
#ifdef NO_NEWGRP
	{ "newgrp",	SYSNEWGRP },
#endif
#endif

	{ "pwd",	SYSPWD },
	{ "read",	SYSREAD	},
	{ "readonly",	SYSRDONLY },
	{ "return",	SYSRETURN },
	{ "set",	SYSSET	},
	{ "shift",	SYSSHFT	},
	{ "test",	SYSTST },
	{ "times",	SYSTIMES },
	{ "trap",	SYSTRAP	},
	{ "type",	SYSTYPE },


#ifndef RES		
	{ "ulimit",	SYSULIMIT },
	{ "umask",	SYSUMASK },
#endif

	{ "unset", 	SYSUNS },
	{ "wait",	SYSWAIT	}
};

#ifdef RES
	int no_commands = 26;
#else
	int no_commands = 27;
#endif
