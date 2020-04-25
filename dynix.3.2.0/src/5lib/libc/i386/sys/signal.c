/*
	signal -- system call emulation for 4.2BSD (i386 version)

	last edit:	13-Jan-1984	D A Gwyn

	NOTE:  Although this module is VAX-specific, it should be
	possible to adapt it to other fairly clean implementations of
	4.2BSD.  The difficulty lies in avoiding the automatic restart
	of certain system calls when the signal handler returns.  I use
	here a trick first described by Donn Seeley of UCSD Chem. Dept.
*/

#include	<errno.h>
#include	<signal.h>
#include	<syscall.h>

extern int	_sigvec();
extern long	_sigsetmask();

extern		etext;

#if lint
#define	BADSIG	(int (*)())0		/* fake "previous handler" */
#else
#define	BADSIG	(int (*)())-1		/* fake "previous handler" */
#endif

typedef int	bool;			/* Boolean data type */
#define	false	0
#define	true	1

#define	SZSVC		2		/* size of system-call instruction */
#define	FLAGS_CF	0x00000001	/* carry flag */

static int	(*handler[NSIG])() =	/* "current handler" memory */
	{
	BADSIG				/* initially, unknown state */
	};
static bool	inited = false;		/* for initializing above */

typedef struct
	{
	int		(*sv_handler)();/* signal handler */
	long		sv_mask;	/* signal mask to apply */
	bool		sv_onstack;	/* take on signal stack */
	}	sigvec;			/* for _sigvec() */

static int	catchsig();

int	(*
signal( sig, func )			/* returns previous handler */
	)()
	register int	sig;		/* signal affected */
	register int	(*func)();	/* new handler */
	{
	register int	(*retval)();	/* previous handler value */
	sigvec		oldsv;		/* previous state */
	sigvec		newsv;		/* state being set */

	if ( func >= (int (*)())&etext
       /** && func != SIG_DFL && func != SIG_IGN **/
	   )	{
		errno = EFAULT;
		return BADSIG;		/* error */
		}

	/* cancel pending signals */
	newsv.sv_handler = SIG_IGN;
	newsv.sv_mask = 0L;
	newsv.sv_onstack = false;
	if ( _sigvec( sig, &newsv, &oldsv ) != 0 )
		return BADSIG;		/* error */

	/* C language provides no good way to initialize handler[] */
	if ( !inited )			/* once only */
		{
		register int	i;

		for ( i = 1; i < NSIG; ++i )
			handler[i] = BADSIG;	/* initialize */

		inited = true;
		}

	/* the first time for this sig, get state from the system */
	if ( (retval = handler[sig - 1]) == BADSIG )
		retval = oldsv.sv_handler;

	handler[sig - 1] = func;	/* keep track of state */

	if ( func == SIG_DFL )
		newsv.sv_handler = SIG_DFL;
	else if ( func != SIG_IGN )
		newsv.sv_handler = catchsig;	/* actual sig catcher */

	if ( func != SIG_IGN		/* sig already being ignored */
	  && _sigvec( sig, &newsv, (sigvec *)0 ) != 0
	   )
		return BADSIG;		/* error */

	return retval;			/* previous handler */
	}


/* PC will be pointing at a syscall if it is to be restarted: */
typedef unsigned char	opcode;		/* one byte long */
#define	SYSCALL		((opcode)0xCD)	/* i386 int instruction */

typedef struct				/* MACHINE specific format */
	{
	int		sc_onstack;	/* sigstack state to restore */
	int		sc_mask;	/* signal mask to restore */
	int		sc_sp;		/* sp to restore */
	int		sc_modpsr;	/* psr and mod to restore */
	opcode		*sc_pc;		/* pc to retore */
	}	sigcontext;		/* interrupted context */


/*ARGSUSED*/
static int
catchsig( sig, code, scp, sr2, sr1, sr0 )	/* signal interceptor */
	register int		sig;	/* signal number */
	long			code;	/* code for SIGILL, SIGFPE */
	register sigcontext	*scp;	/* -> interrupted context */
	int	sr2, sr1, sr0;
	{
	register int		(*uhandler)();	/* user handler */
	register opcode		*pc;	/* for snooping instructions */
	sigvec			oldsv;	/* previous state */
	sigvec			newsv;	/* state being set */

	/* at this point, sig is blocked */

	uhandler = handler[sig - 1];

	/* UNIX System V usually wants the state reset to SIG_DFL */
	if ( sig != SIGILL && sig != SIGTRAP /* && sig != SIGPWR */ )
		{
		handler[sig - 1] = newsv.sv_handler = SIG_DFL;
		newsv.sv_mask = 0L;
		newsv.sv_onstack = false;
		(void)_sigvec( sig, &newsv, &oldsv );
		}

	(void)_sigsetmask( scp->sc_mask );	/* restore old mask */

	/* at this point, sig is not blocked, usually have SIG_DFL;
	   a longjmp may safely be taken by the user signal handler */

	(*uhandler)( sig, code, scp, sr2, sr1, sr0 );		/* user signal handler */

	/* must now avoid restarting certain system calls */
	pc = scp->sc_pc;
	if ( *pc == SYSCALL ) {
		if (sr0 == ( SYSV_read|(SYS_BSD<<16))
		 || sr0 == ( SYS_write|(SYS_BSD<<16))
		 || sr0 == ( SYS_ioctl|(SYS_BSD<<16))
		 || sr0 == ( SYS_wait |(SYS_BSD<<16))) {
			scp->sc_pc += SZSVC;
			scp->sc_modpsr |= FLAGS_CF;
			sr0 = EINTR;
		}
	}

	/* return here restores interrupted context */
	}
