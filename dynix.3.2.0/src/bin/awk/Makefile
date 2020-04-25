# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 2.2 86/06/17 $
#
CFLAGS=	-O $(DEBUG) $(INCLUDE)
YFLAGS=	-d
#

OBJS=	awk.lx.o awk.g.o b.o main.o token.o tran.o lib.o run.o parse.o\
	proctab.o freeze.o

all:	awk

awk:&	token.c $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lm -o awk

${OBJS}: awk.h awk.def

y.tab.h: awk.g.o

awk.h:	y.tab.h
	cp y.tab.h awk.h

token.c: awk.h tokenscript
	chmod +w token.c
	e - <tokenscript

install:
	install -s -m 755 awk ${DESTDIR}/bin

lint:
	lint -spu b.c main.c token.c tran.c run.c lib.c parse.c -lm |\
		egrep -v '^(error|free|malloc)'

proctab.c:	proc
	proc >proctab.c

## these really want to use "cc" because they *run* programs ##
proc:	awk.h proc.o host_token.o
	cc -o proc proc.o host_token.o

proc.o:	proc.c
	cc -c -O proc.c

host_token.o: token.c
	cp token.c host_token.c
	cc -c -O host_token.c
	rm -f host_token.c

clean:
	rm -f *.o t.* *temp* *.out *junk* y.tab.* errs \
		awk.h proc proctab.c awk yacc*
