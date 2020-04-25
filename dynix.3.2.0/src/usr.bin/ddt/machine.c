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
 * machine.c: version 1.25 of 10/6/83
 * 
 */
# ifndef lint
static char rcsid[] = "$Header: machine.c 2.0 86/01/28 $";
# endif

/* machine interface: local(use ptrace syscall) or remote(use remote monitor) */

/*
 * host defines
 */

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>

/* 
 * target defines
 */

#include "main.h"
#include "sym.h"
#include "display.h"
#include "parse.h"
#include "error.h"
#include "scan.h"
#include "bpt.h"
#include "machine.h"

#define MAXARGS 15
#define MAXLINE 120

extern char **environ;
extern int errno;
extern int pid;

char *fileptr[MAXARGS] = { imagename, 0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
char *inefile;	/* input stdin file */	
char *outefile;	/* output stdout file */	

initexec()
{
	int i = 0;

	fileptr[i] = imagename;
	for (i = 1; i < MAXARGS; i++) {
		if (fileptr[i] != 0) {
		    free(fileptr[i]);
		    fileptr[i] = 0;
		}
	}
}

getexecargs() 
{
char *cptr = scanbuffer;
char *argbegin;
char inoutfile = 0;
int i = 1, xsize = 0;
	inefile = 0;
	outefile = 0;
	if (*cptr == 0) {
		printf("\tbad arg scan\r\n");
		processabort();
	}
	do {
		cptr++;
		if (*cptr == 0) {
			printf("\tbad arg scan\r\n");
			processabort();
		}
		if (*cptr == 'g') return;
	} while (*cptr != 'G');
	cptr++;
	if ((*cptr == '\r') || (*cptr == '\n')) return;
	do {
	    if (*cptr == 0) break;
	    while (*cptr == ' ') cptr++;
	    if ((*cptr == '\r') || (*cptr == '\n')) break;
	    if ((*cptr == '<') || (*cptr == '>')) {
		inoutfile = *cptr;
		cptr++;
		if (*cptr == 0) break;
		while (*cptr == ' ') cptr++;
		if ((*cptr == '\r') || (*cptr == '\n')) break;
	    }
	    argbegin = cptr;
	    while ((*cptr != ' ') && (*cptr != '\r') && (*cptr != 0)
		   && (*cptr != '\n')) cptr++;
	    if ((*cptr == '\r') || (*cptr == 0) || (*cptr == '\n'))
	   			*(cptr+1) = '\r';
	    *cptr = '\0';
	    cptr++;
	    xsize = strlen(argbegin);
	    if (xsize <= 0) break;
	    if (inoutfile != 0) {
		if (inoutfile == '<') {
		    if (inefile != 0) free(inefile);
		    inefile = (char *) malloc((xsize + 1));
		    strcpy(inefile,argbegin);
		} else {
		    if (outefile != 0) free(outefile);
		    outefile = (char *) malloc((xsize + 1));
		    strcpy(outefile,argbegin);
		}
		inoutfile = 0;
	    } else {
		if (fileptr[i] != 0) free(fileptr[i]);
		fileptr[i] = (char *) malloc((xsize + 1));
		strcpy(fileptr[i],argbegin);
		i++;
	    }
	} while ((*cptr != '\r') && (*cptr != '\n'));
	for (; i < MAXARGS; i++) {
		if (fileptr[i] != 0) {
		    free(fileptr[i]);
		    fileptr[i] = 0;
		}
	}
}

printargs()
{
int i;
    printf("\r\n");
    for (i=0; i < MAXARGS; i++) {
	if (fileptr[i] == 0) continue;
	printf("%s ",fileptr[i]);
    }
    printf("\r\n");
}

startup()
{
	int i;

	printargs();
	if ((pid = fork()) == 0) {
		close(imagefd);
		stty(0, &userstty);
		if (inefile != 0) {
			close(0);
			i = open(inefile,0);
			if (i < 0) {
			    printf("can't open %s\r\n",inefile);
			    exit(i);
			}
		}
		if (outefile != 0) {
			close(1);
			i = creat(outefile,0666);
			if (i < 0) {
			    printf("can't creat %s\r\n",outefile);
			    exit(i);
			}
		}
		machine(INIT,0,0,0);
		execv(imagename,fileptr);
		perror("exec fails");
		exit(0);
	}
}

waitfor(waitrst)
int *waitrst;
{
	return(wait(waitrst));
}

machine(operation, processid, location, value)
{
#ifndef DEBUG
	return(ptrace(operation, processid, location, value));
#else
	extern int debug_ptrace;
	int retval = ptrace(operation, processid, location, value);
	if (debug_ptrace)
		printf("\r\nptrace(%d, %d, 0x%x, 0x%x)==0x%x\r\n",
			operation, processid, location, value, retval);
	return(retval);
#endif DEBUG
}
