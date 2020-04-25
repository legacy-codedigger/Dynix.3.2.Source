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
static char rcsid[] = "$Header: doname.c 2.5 86/05/30 $";
#endif

#include "defs"
#include <strings.h>
#include <signal.h>

/*  BASIC PROCEDURE.  RECURSIVE.  */

/*
p->done = 0   don't know what to do yet
p->done = 1   file in process of being updated
p->done = 2   file already exists in current state
p->done = 3   file make failed
*/


doname(p, reclevel, tval, nowait)
register struct nameblock *p;
int reclevel;
TIMETYPE *tval;
int nowait;
{
int errstat;
int okdel1;
int didwork;
TIMETYPE td, td1, tdep, ptime, ptime1, prestime();
register struct depblock *q;
struct depblock *qtemp, *srchdir(), *suffp, *suffp1;
struct nameblock *p1, *p2;
struct shblock *implcom, *explcom;
register struct lineblock *lp;
struct lineblock *lp1, *lp2;
char sourcename[BUFSIZ], prefix[BUFSIZ], temp[BUFSIZ], concsuff[20];
char *pnamep, *p1namep, *cp;
char *mkqlist();
struct chain *qchain, *appendq();

if( proclimit == 1 )
	nowait = 0;
if(p == 0)
	{
	*tval = 0;
	return(0);
	}

if(dbgflag)
	{
	printf("doname(%s,%d,%d)\n",p->namep,reclevel,nowait);
	fflush(stdout);
	}

if(p->done > 0)
	{
	*tval = p->modtime;
	return(p->done == 3);
	}

errstat = 0;
tdep = 0;
implcom = 0;
explcom = 0;
ptime = exists(p); 
ptime1 = 0;
didwork = NO;
p->done = 1;	/* avoid infinite loops */

qchain = NULL;

/* Expand any names that have embedded metacharaters */

for(lp = p->linep ; lp ; lp = lp->nxtlineblock)
	for(q = lp->depp ; q ; q=qtemp )
		{
		qtemp = q->nxtdepblock;
		expand(q);
		}

/* make sure all dependents are up to date */

for(lp = p->linep ; lp ; lp = lp->nxtlineblock)
	{
	td = 0;
	for(q = lp->depp ; q ; q = q->nxtdepblock)
		{
		errstat += doname(q->depname, reclevel+1, &td1, q->nowait);
		if(dbgflag)
		    printf("TIME(%s)=%ld\n", q->depname->namep, td1);
		if(td1 > td) td = td1;
		if(ptime < td1)
			qchain = appendq(qchain, q->depname->namep);
		}
	if(p->septype == SOMEDEPS)
		{
		if(lp->shp!=0)
		     if( ptime<td || (ptime==0 && td==0) || lp->depp==0)
			{
			okdel1 = okdel;
			okdel = NO;
			setvar("@", p->namep);
			setvar("?", mkqlist(qchain) );
			qchain = NULL;
			if( !questflag )
				errstat += docom(lp->shp,nowait,reclevel);
			setvar("@", (char *) NULL);
			okdel = okdel1;
			ptime1 = prestime();
			didwork = YES;
			}
		}

	else	{
		if(lp->shp != 0)
			{
			if(explcom)
				fprintf(stderr, "Too many command lines for `%s'\n",
					p->namep);
			else	explcom = lp->shp;
			}

		if(td > tdep) tdep = td;
		}
	}

/* Look for implicit dependents, using suffix rules */

for(lp = sufflist ; lp ; lp = lp->nxtlineblock)
    for(suffp = lp->depp ; suffp ; suffp = suffp->nxtdepblock)
	{
	pnamep = suffp->depname->namep;
	if(suffix(p->namep , pnamep , prefix))
		{

		srchdir( concat(prefix,"*",temp) , NO, (struct depblock *) NULL);
		for(lp1 = sufflist ; lp1 ; lp1 = lp1->nxtlineblock)
		    for(suffp1=lp1->depp ; suffp1 ; suffp1 = suffp1->nxtdepblock)
			{
			p1namep = suffp1->depname->namep;
			if( (p1=srchname(concat(p1namep, pnamep ,concsuff))) &&
			    (p2=srchname(concat(prefix, p1namep ,sourcename))) )
				{
				errstat += doname(p2, reclevel+1, &td, NO);
				if(ptime < td)
					qchain = appendq(qchain, p2->namep);
if(dbgflag) printf("TIME(%s)=%ld\n", p2->namep, td);
				if(td > tdep) tdep = td;
				setvar("*", prefix);
				if (p2->alias) setvar("<", copys(p2->alias));
				else setvar("<", copys(p2->namep));
				for(lp2=p1->linep ; lp2 ; lp2 = lp2->nxtlineblock)
					if(implcom = lp2->shp) break;
				goto endloop;
				}
			}
		cp = rindex(prefix, '/');
		if (cp++ == 0)
			cp = prefix;
		setvar("*", cp);
		}
	}

endloop:


if(errstat==0 && (ptime<tdep || (ptime==0 && tdep==0) ) )
	{
	ptime = (tdep>0 ? tdep : prestime() );
	setvar("@", p->namep);
	setvar("?", mkqlist(qchain) );
	if(explcom)
		errstat += docom(explcom,nowait,reclevel);
	else if(implcom)
		errstat += docom(implcom,nowait,reclevel);
	else if(p->septype == 0) {
		if(p1=srchname(".DEFAULT"))
			{
			if (p->alias) setvar("<", p->alias);
			else setvar("<", p->namep);
			for(lp2 = p1->linep ; lp2 ; lp2 = lp2->nxtlineblock)
				if(implcom = lp2->shp)
					{
					errstat += docom(implcom,nowait,reclevel);
					break;
					}
			}
		else if(keepgoing)
			{
			printf("Don't know how to make %s\n", p->namep);
			++errstat;
			}
		else
			fatal1(" Don't know how to make %s", p->namep);
	} else {
		if( !nowait )
			waitlevel(reclevel); /* in the event there is nothing */
	}

	setvar("@", (char *) NULL);
	if(noexflag || nowait || (ptime = exists(p)) == 0)
		ptime = prestime();
	}

else if(errstat!=0 && reclevel==0)
	printf("`%s' not remade because of errors\n", p->namep);

else if(!questflag && reclevel==0  &&  didwork==NO)
	printf("`%s' is up to date.\n", p->namep);

if(questflag && reclevel==0)
	exit(ndocoms>0 ? -1 : 0);

p->done = (errstat ? 3 : 2);
if(ptime1 > ptime) ptime = ptime1;
p->modtime = ptime;
*tval = ptime;
return(errstat);
}

docom(q,nowait,reclevel)
struct shblock *q;
int nowait,reclevel;
{
char *s;
struct varblock *varptr();
int ign, nopr;
char string[OUTMAX];
char string2[OUTMAX];

++ndocoms;
if(questflag)
	return(NO);

if(touchflag)
	{
	s = varptr("@")->varval;
	if(!silflag)
		printf("touch(%s)\n", s);
	if(!noexflag)
		touch(YES, s);
	return NO;
	}

for( ; q ; q = q->nxtshblock )
	{
	subst(q->shbp,string2);
	fixname(string2, string);

	ign = ignerr;
	nopr = NO;
	for(s = string ; *s=='-' || *s=='@' ; ++s)
		if(*s == '-')  ign = YES;
		else nopr = YES;

	if( docom1(s, ign, nopr, nowait&&!q->nxtshblock, reclevel) && !ign)
		if(keepgoing)
			return(YES);
		else	fatal( (char *) NULL);
	}
return(NO);
}

docom1(comstring, nohalt, noprint, nowait, reclevel)
register char *comstring;
int nohalt, noprint, nowait, reclevel;
{

if(comstring[0] == '\0') return(0);

if(noexflag) {
	if( !silflag ) printf("\t%s\n", comstring);
	fflush(stdout);
	return(0);
	}

return( dosys(comstring, nohalt, noprint, nowait, reclevel) );
}


/*
   If there are any Shell meta characters in the name,
   expand into a list, after searching directory
*/

expand(q)
register struct depblock *q;
{
register char *s;
char *s1;
struct depblock *p, *srchdir();

if (q->depname == NULL)
	return;
s1 = q->depname->namep;
for(s=s1 ; ;) switch(*s++)
	{
	case '\0':
		return;

	case '*':
	case '?':
	case '[':
		if( p = srchdir(s1 , YES, q->nxtdepblock) )
			{
			q->nxtdepblock = p;
			q->depname = 0;
			}
		return;
	}
}
