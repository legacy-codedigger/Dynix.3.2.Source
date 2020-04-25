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
static char rcsid[] = "$Header: dosys.c 2.5 1991/06/16 23:33:47 $";
#endif

#include "defs"
#ifdef CCS
#include "/usr/include/signal.h"
#else
#include <signal.h>
#endif

int intrupt();

struct process aproc;

char derr[ 1024 ];	/* if doexec or doshell fails, error message is placed here */

dosys(comstring, nohalt, noprint, nowait, reclevel)
register char *comstring;
int nohalt, noprint, nowait, reclevel;
{
int status, pid;
register struct process *procp;

/* make sure fewer than proclimit processes are running */

while(proclive >= proclimit)
	{
	enbint(SIG_IGN);
	waitproc(&status);
	enbint(intrupt);
	}

waitlevel(reclevel+1);

if( !silflag && !noprint )
	printf("\t%s%s", prompt, comstring);

pid = metas(comstring) ? doshell(comstring,nohalt) : doexec(comstring);
if( pid == -1 )
	fatal("\nToo many processes - Can't fork.\n");

procp = (struct process *)malloc(sizeof (struct process));
procp->link = aproc.link; aproc.link = procp;
procp->pid = pid;
procp->nohalt = nohalt;
procp->nowait = nowait;
procp->reclevel = reclevel;
++proclive;

if( !silflag && !noprint ) {
	if(nowait)
		printf(" &%d\n", procp->pid);
	else
		printf("\n");
}
fflush(stdout);

if (*derr != '\0') {
	fprintf(stderr, "Make: %s.  Stop.\n", derr);
	*derr = '\0';
}

if(nowait) {
	return 0;
}

return waitlevel(reclevel);
}

waitlevel(reclevel) /*  wait till none of the procs above reclevel is live */
int reclevel;
{
	int npending, status, totstatus;
	register struct process *procp;

	totstatus = 0;
	npending = 0;

	for(procp = aproc.link; procp; procp = procp->link)
		if( procp->reclevel >= reclevel )
			++npending;
	enbint(SIG_IGN);
	if(dbgflag)
		printf("waitlevel (%d) %d pending\n", reclevel, npending);

	while(npending > 0)
		{
		if(waitproc(&status) >= reclevel)
			--npending;
		totstatus |= status;
		}

	enbint(intrupt);
	return totstatus;
}

waitproc(statp)
register int *statp;
{
	register struct process *procp, *procl;
	char junk[50];
	int pid, level, sig;
	extern char *sys_siglist[];

	pid = wait(statp);
	if(pid == -1) {
		perror("wait ret");
		exit(1);
	}
	if(dbgflag)
		fprintf(stderr, "process %d done, status = %d\n", pid, *statp);
	for(procl = &aproc, procp = aproc.link; procp; procp = (procl=procp)->link)
		if(procp->pid == pid) {
			--proclive;
			level = procp->reclevel;
			procl->link = procp->link;
			free(procp);
			if(*statp == 0)
				return level;	/* reclevel */
			if(procp->nowait)
				printf("\n%d: ", pid);
			if( sig = (*statp & 0177) ) {
				if (sig < NSIG && sys_siglist[sig] != NULL &&
				    *sys_siglist[sig] != '\0')
					printf("*** %s", sys_siglist[sig]);
				else
					printf("*** Signal %d", sig);
				if (*statp & 0200)
					printf(" - core dumped");
			} else {
				printf("*** Error code %d", *statp>>8 );
			}
			printf(procp->nohalt ? " (ignored)\n" : "\n");
			fflush(stdout);
			if(!keepgoing && !procp->nohalt)
				fatal("");
			return level;	/* reclevel */
		}
	sprintf(junk, "spurious return from process %d", pid);
	fatal(junk);
	/*NOTREACHED*/
}


metas(s)   /* Are there are any  Shell meta-characters? */
register char *s;
{
register char c;

while( (funny[c = *s++] & META) == 0 )
	;
return( c );
}

doshell(comstring,nohalt)
char *comstring;
int nohalt;
{
int pid;
#ifdef SHELLENV
char *getenv(), *rindex();
char *shellcom = getenv("SHELL");
char *shellstr;
#endif
if((pid = vfork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();

#ifdef SHELLENV
	if (shellcom == 0) shellcom = SHELLCOM;
	shellstr = rindex(shellcom, '/') + 1;
	execl(shellcom, shellstr, (nohalt ? "-c" : "-ce"), comstring, 0);
#else
	execl(SHELLCOM, "sh", (nohalt ? "-c" : "-ce"), comstring, 0);
#endif
	strcpy(derr, "Couldn't load Shell");
	_exit(1);
	}

return pid;
}

/*
 * Close open directory files before exec'ing
 */
doclose()
{
register struct dirhdr *od;

/* We have done a vfork, so we can't do anything 
   to stomp on our parent's address space.  */
for (od = firstod; od; od = od->nxtopendir)
	if (od->dirfc != NULL)
		/*
		 * vfork kludge...
		 * we cannot call closedir since this will modify
		 * the parents data space; just call close directly.
		 */
		close(od->dirfc->dd_fd);
}



doexec(str)
register char *str;
{
register char *t, *tend;
char **argv;
register char **p;
int nargs;
int pid;

while( *str==' ' || *str=='\t' )
	++str;
if( *str == '\0' )
	return(-1);	/* no command */

nargs = 1;
for(t = str ; *t ; )
	{
	++nargs;
	while(*t!=' ' && *t!='\t' && *t!='\0')
		++t;
	if(*t)	/* replace first white space with \0, skip rest */
		for( *t++ = '\0' ; *t==' ' || *t=='\t'  ; ++t)
			;
	}

/* now allocate args array, copy pointer to start of each string,
   then terminate array with a null
*/
p = argv = (char **) ckalloc(nargs*sizeof(char *));
tend = t;
for(t = str ; t<tend ; )
	{
	*p++ = t;
	while( *t )
		++t;
	do	{
		++t;
		} while(t<tend && (*t==' ' || *t=='\t') );
	}
*p = NULL;

if((pid = vfork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();
	enbint(intrupt);
	execvp(str, argv);
	sprintf(derr, "Cannot load %s", str);
	_exit(1);
	}

free( (char *) argv);
return pid;
}

#ifdef CCS
#include "/usr/include/errno.h"

#include "/usr/include/sys/stat.h"
#else
#include <errno.h>

#include <sys/stat.h>
#endif



touch(force, name)
int force;
char *name;
{
struct stat stbuff;
char junk[1];
int fd;

if( stat(name,&stbuff) < 0)
	if(force)
		goto create;
	else
		{
		fprintf(stderr, "touch: file %s does not exist.\n", name);
		return;
		}

if(stbuff.st_size == 0)
	goto create;

if( (fd = open(name, 2)) < 0)
	goto bad;

if( read(fd, junk, 1) < 1)
	{
	close(fd);
	goto bad;
	}
lseek(fd, 0L, 0);
if( write(fd, junk, 1) < 1 )
	{
	close(fd);
	goto bad;
	}
close(fd);
return;

bad:
	fprintf(stderr, "Cannot touch %s\n", name);
	return;

create:
	if( (fd = creat(name, 0666)) < 0)
		goto bad;
	close(fd);
}
