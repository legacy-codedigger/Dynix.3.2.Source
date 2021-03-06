struct argent *lbot, *np;

#ifndef lint
static char *rcsid =
   "$Header: i386.c 1.2 87/04/08 $";
#endif

/*					-[Mon Mar 21 19:35:50 1983 by jkf]-
 * 	vax.c				$Locker:  $
 * vax specific functions
 *
 * (c) copyright 1982, Regents of the University of California
 */
 
#include "global.h"
#include <signal.h>
#include "vaxframe.h"

/*
 * exarith(mul1,mul2,add,hi,lo)
 *
 * (hi,lo) gets 64 bit product + sum of mul1*mul2+add;
 * routine returns non-zero if product is bigger than 30 bits
 */

struct vl	{ long high; long low; };

long exarith(mul1,mul2,add,hi,lo)
long *hi, *lo;
register long add;
{
	struct vl work;
	register long rlo;

	emul(mul1,mul2,add,&work);
	add = work.high;
	add <<= 2;
	if((rlo = work.low) < 0) add += 2;
	if(rlo & 0x40000000) add += 1;
	*lo = rlo &0x3fffffff;
	(*hi = add);
	if((add==0) || (add!=-1)) return(add);
	*lo = rlo;
	return(0);
}

mmuladd(a,b,c,m)
long a,b,c,m;
{
	long work[2]; char err;
	emul(a,b,c,work);
	ediv(work,m,err);
	return(work[0]);
}

Imuldiv(p1,p2,add,dv,quo,rem)
long p1, p2, add, dv;
long *quo, *rem;
{
	long work[2]; char err;

	emul(p1,p2,add,work);
	*quo = ediv(work,dv, &err);
	*rem = *work;
}

callg_(funct,arglist)
lispval (*funct)();
int *arglist;
{
	abort();
	/*asm("	callg	*8(ap),*4(ap)");*/
}

#include <errno.h>
#define WRITE 4
#define READ 3

#define _read(a,b,c) syscall(READ,a,b,c)
#define _write(a,b,c) syscall(WRITE,a,b,c)

/*C library -- write
  nwritten = write(file, buffer, count);
  nwritten == -1 means error
*/
write(file, buffer, count)
char *buffer;
{
	register lispval handy;
	int retval;
	if((file != 1) || (Vcntlw->a.clb == nil)) goto top;
	/* since ^w is non nil, we do not want to print to the terminal,
	   but we must be sure to return a correct value from the write
	   in case there is no write to ptport
	*/
	retval = count;
	goto skipit;
top:
	retval = _write(file,buffer,count);

skipit:
    if(file==1) {
	handy = Vptport->a.clb;
	if(handy!=nil && TYPE(handy)==PORT && handy->p->_file!=1) {
		fflush(handy->p);
		file = handy->p->_file;
		goto top;
	}
    }
    return(retval);
}

/*
 *
 *nread = read(file, buffer, count);
 *nread ==0 means eof; nread == -1 means error
 *
 */

read(file,buffer,count)
{
	extern int errno;
	register int Size;
again:
	Size = _read(file,buffer,count);
	if ((Size >= 0) || (errno != EINTR)) return(Size);
	if(sigintcnt > 0) sigcall(SIGINT);
	goto again;
}

lispval
Lpolyev()
{
	register int count; 
	register double *handy, *base;
	register struct argent *argp;
	lispval result; int type;
	char *alloca();
	Keepxs();

	count = 2 * (((int) np) - (int) lbot);
	if(count == 0) 
		return(inewint(0));
	if(count == 8)
		return(lbot->val);
	base = handy = (double *) alloca(count);
	for(argp = lbot; argp < np; argp++) {
		while((type = TYPE(argp->val))!=DOUB && type!=INT)
			argp->val = (lispval) errorh2(Vermisc,"%%machine-polyev:non-real arg",nil,TRUE,73,lbot,argp->val);
		if(TYPE(argp->val)==INT) {
			*handy++ = argp->val->i;
		} else
			*handy++ = argp->val->r;
	}
	count = count/sizeof(double) - 2;
	result = newdoub();
	result->r = *base;
	Freexs();
	return(result);
}

lispval
Lrot()
{
	register rot,val;		/* these must be the first registers */
	register struct argent *mylbot = lbot;

	chkarg(2,"rot");
	if((TYPE(mylbot->val) != INT) || (TYPE(mylbot[1].val) != INT))
		errorh2(Vermisc,
		       "Non ints to rot",
		       nil,FALSE,0,mylbot->val,mylbot[1].val);
	val = mylbot[0].val->i;
	rot = mylbot[1].val->i;
	rot = rot % 32 ;	/* bring it down below one byte in size */
	val <<= rot;
	return( inewint(val));
}

/* new version of showstack,
	We will set fp to point where the register fp points.
	Then fp+2 = saved ap
	     fp+4 = saved pc
	     fp+3 = saved fp
	     ap+1 = first arg
	If we find that the saved pc is somewhere in the routine eval,
   then we print the first argument to that eval frame. This is done
   by looking one beyond the saved ap.
*/
lispval
Lshostk()
{	lispval isho();
	return(isho(1));
}
static lispval
isho(f)
int f;
{
	register struct machframe *myfp; register lispval handy;
	int **fp;	/* this must be the first local */
	int virgin=1;
	lispval linterp();
	lispval _qfuncl(),tynames();	/* locations in qfuncl */
	extern int plevel,plength;

	if(TYPE(Vprinlevel->a.clb) == INT)
	{ 
	   plevel = Vprinlevel->a.clb->i;
	}
	else plevel = -1;
	if(TYPE(Vprinlength->a.clb) == INT)
	{
	    plength = Vprinlength->a.clb->i;
	}
	else plength = -1;

	if(f==1)
		printf("Forms in evaluation:\n");
	else
		printf("Backtrace:\n\n");

	myfp = (struct machframe *) (&fp +1);	/* point to current frame */

	while(TRUE)
	{
	    if( (myfp->pc > eval  &&  		/* interpreted code */
		 myfp->pc < popnames)
		||
		(myfp->pc > Lfuncal &&		/* compiled code */
		 myfp->pc < linterp)  )
	    {
	      if(((int) myfp->ap[0]) == 1)		/* only if arg given */
	      { handy = (myfp->ap[1]);
		if(f==1)
			printr(handy,stdout), putchar('\n');
		else {
			if(virgin)
				virgin = 0;
			else
				printf(" -- ");
			printr((TYPE(handy)==DTPR)?handy->d.car:handy,stdout);
		}
	      }

	    }

	    if(myfp > myfp->fp) break;	/* end of frames */
	    else myfp = myfp->fp;
	}
	putchar('\n');
	return(nil);
}

/*
 *
 *	(baktrace)
 *
 * baktrace will print the names of all functions being evaluated
 * from the current one (baktrace) down to the first one.
 * currently it only prints the function name.  Planned is a
 * list of local variables in all stack frames.
 * written by jkf.
 *
 */
lispval
Lbaktrace()
{
	isho(0);
}

/*
 * (int:showstack 'stack_pointer)
 * return
 *   nil if at the end of the stack or illegal
 *   ( expresssion . next_stack_pointer) otherwise
 *   where expression is something passed to eval
 * very vax specific
 */
lispval
LIshowstack()
{
    int **fp;	/* must be the first local variable */
    register lispval handy;
    register struct machframe *myfp;
    lispval retval, Lfuncal(), Ifuncal();
    Savestack(2);
    
    chkarg(1,"int:showstack");

    if((TYPE(handy=lbot[0].val) != INT) && (handy != nil))
        error("int:showstack non fixnum arg", FALSE);

    if(handy == nil)
        myfp = (struct machframe *) (&fp +1);
    else
        myfp = (struct machframe *) handy->i;
	
    if((int ***)myfp <= &fp) error("int:showstack illegal stack value",FALSE);
    while(myfp > 0)
    {
        if( (myfp->pc > eval  &&  		/* interpreted code */
            myfp->pc < popnames)
	    ||
	    (myfp->pc > Ifuncal &&		/* compiled code */
	    myfp->pc < Lfuncal)  )
        {
	    if(((int) myfp->ap[0]) == 1)	/* only if arg given */
	    {
		handy = (lispval)(myfp->ap[1]);	/* arg to eval */

		protect(retval=newdot());
		retval->d.car = handy;
		if(myfp > myfp->fp)
		    myfp = 0;	/* end of frames */
		else
		    myfp = myfp->fp;
		retval->d.cdr = inewint(myfp);
		return(retval);
	    }
	}
	if(myfp > myfp->fp)
	     myfp = 0;	/* end of frames */
	else
	     myfp = myfp->fp;

    }
    return(nil);
}
#include "frame.h"
/*
 * this code is very similar to ftolsp.
 * if it gets revised, so should this.
 */
lispval
dothunk(func,count,arglist)
lispval func;
long count;
register long *arglist;
{

	lispval save;
	pbuf pb;
	Savestack(1);

	if(errp->class==F_TO_FORT)
		np = errp->svnp;
	errp = Pushframe(F_TO_LISP,nil,nil);
	lbot = np;
	np++->val = func;
	arglist++;
	for(; count > 0; count--)
		np++->val = inewint(*arglist++);
	save = Lfuncal();
	errp = Popframe();
	Restorestack();
	return(save);
}
/*
_thcpy:
	movl	(sp),r0
	pushl	ap
	pushl	(r0)+
	pushl	(r0)+
	calls	$3,_dothunk
	ret */
static char fourwords[] = "0123456789012345";

lispval
Lmkcth()
{
	register struct argent *mylbot = lbot;
	register struct thunk {
		short	mask;
		short 	jsri;
		char	*thcpy;
		long	count;
		lispval func;
	} *th;
	extern char thcpy[];

	chkarg(2,"make-c-thunk");
	th = (struct thunk *)pinewstr(fourwords);
	th->mask = 0;
	th->jsri = 0x9f16;
	th->thcpy = thcpy;
	th->func = mylbot->val;
	th->count = mylbot[1].val->i;

	return((lispval)th);
}

myfrexp()
{
	abort();
}
