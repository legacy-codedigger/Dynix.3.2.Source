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

# $Header: Makefile 2.3 90/02/13 $
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.3 (Berkeley) 1/11/86
#
# Berkeley Mail
#
CFLAGS=	-O -DVMUNIX -DV7 $(INCLUDE)
CTAGS=	/usr/ucb/ctags -w
VPRINT=	/usr/ucb/print
RM=	rm -f
OBJS=	version.o aux.o cmd1.o cmd2.o cmd3.o cmdtab.o collect.o \
	config.o edit.o fio.o \
	getname.o head.o v7.local.o lex.o list.o main.o \
	names.o optim.o popen.o quit.o send.o strings.o temp.o tty.o \
	vars.o lock.o
SRCS=	aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c config.c edit.c fio.c \
	getname.c head.c v7.local.c lex.c list.c main.c \
	names.c optim.c popen.c quit.c send.c strings.c temp.c tty.c vars.c \
	version.c lock.c
HDRS=	rcv.h configdefs.h def.h glob.h v7.local.h local.h
ALL=	Mail fmt
S = $(SRCS) $(HDRS)

all:	$(ALL)

Mail:	$& $(OBJS)
	$(CC) -o Mail $(OBJS) $(LIBES)

install:
	install -s Mail fmt $(DESTDIR)/usr/ucb
	cd misc && install -c -m 644 Mail.* $(DESTDIR)/usr/lib
	cd $(DESTDIR)/usr/ucb && rm -f mail && ln Mail mail

version.o:	version.c
	$(CC) $(CFLAGS) -c -R version.c

$S:
# 	sccs get $@;

srcs:	$S

tags:	$(SRCS)
	${CTAGS} $(SRCS);

clean:
	rm -f Mail fmt *.o strings xhead.c

lint:
	lint $(CFLAGS) $(SRCS)

fmt:	fmt.o xhead.o
	$(CC) $(CFLAGS) fmt.o xhead.o -o fmt

fmt.o:	fmt.c
	$(CC) $(CFLAGS) -c fmt.c

xhead.c:
	ln -s head.c xhead.c

xhead.o: xhead.c
	$(CC) $(CFLAGS) -c xhead.c

cmdtab.o: cmdtab.c
	$(CC) -R -c $(CFLAGS) cmdtab.c

print:	$S fmt.c
	@$(VPRINT) Makefile rcv.h def.h glob.h local.h v7.local.h $(SRCS) fmt.c

wc:
	@wc rcv.h def.h glob.h local.h v7.local.h $(SRCS)

sc:
	@grep -c \; rcv.h def.h glob.h local.h v7.local.h $(SRCS)
