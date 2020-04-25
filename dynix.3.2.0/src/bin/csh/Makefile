# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 2.3 1991/07/26 00:27:12 $
#

#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley Software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.10 (Berkeley) 5/19/88
#
# C Shell with process control; VM/UNIX VAX Makefile
# Bill Joy UC Berkeley; Jim Kulp IIASA, Austria
#
# To profile, put -DPROF in DEFS and -pg in CFLAGS, and recompile.

DEFS	= -DTELL -DVFORK -Ddebug -DFILEC ${INCLUDE}
CFLAGS	= ${DEFS} -O
XSTR	= /usr/ucb/xstr
CXREF	= /usr/ucb/cxref
VGRIND	= csh /usr/ucb/vgrind
CTAGS	= /usr/ucb/ctags

SRCS=	alloc.c doprnt.c sh.c sh.char.c sh.dir.c sh.dol.c sh.err.c \
	sh.exec.c sh.exp.c sh.file.c sh.func.c sh.glob.c sh.hist.c \
	sh.init.c sh.lex.c sh.misc.c sh.parse.c sh.print.c sh.proc.c \
	sh.sem.c sh.set.c sh.time.c
OBJS=	alloc.o doprnt.o sh.o sh.char.o sh.dir.o sh.dol.o sh.err.o \
	sh.exec.o sh.exp.o sh.file.o sh.func.o sh.glob.o sh.hist.o \
	sh.init.o sh.lex.o sh.misc.o sh.parse.o sh.print.o sh.proc.o \
	sh.sem.o sh.set.o sh.time.o

# Special massaging of C files for sharing of strings
.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	@sed '/\*rcsid/d' < x.c > $*.x.c
	${CC} -c ${CFLAGS} $*.x.c 
	@rm -f x.c $*.x.c
	@mv $*.x.o $*.o

all:	csh

# strings.o must be last since it can change when previous files compile
csh: ${OBJS} sh.local.h strings.o
	rm -f csh
	${CC} ${CFLAGS} ${OBJS} strings.o -o $@

strings:
	touch strings
	chmod +w strings

# strings.o, sh.init.o, and sh.char.o are specially processed to be shared
strings.o: strings
	${XSTR}
	${CC} -c -R xs.c
	@rm -f xs.c
	@mv xs.o strings.o

sh.char.o sh.init.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	${CC} ${CFLAGS} -c -R x.c
	@rm -f x.c
	@mv x.o $*.o

lint:
	lint ${CFLAGS} ${SRCS}

print:
	@pr READ_ME
	@pr Makefile Makefile.*
	@(size -l a.out; size *.o) | pr -h SIZES
	@${CXREF} sh*.c | pr -h XREF
	@ls -l | pr 
	@pr sh*.h [a-rt-z]*.h sh*.c alloc.c

vprint:
	@pr -l84 READ_ME TODO
	@pr -l84 Makefile Makefile.*
	@(size -l a.out; size *.o) | pr -l84 -h SIZES
	@${CXREF} sh*.c | pr -l84 -h XREF
	@ls -l | pr -l84
	@${CXREF} sh*.c | pr -l84 -h XREF
	@pr -l84 sh*.h [a-rt-z]*.h sh*.c alloc.c

vgrind:
	@cp /dev/null index
	@-mkdir grind
	for i in *.h; do ${VGRIND} -t -h "C Shell" $$i >grind/$$i.t; done
	for i in *.c; do ${VGRIND} -t -h "C Shell" $$i >grind/$$i.t; done
	${VGRIND} -t -x -h Index index >grind/index.t

install:
	install -s csh ${DESTDIR}/bin/csh

clean:
	rm -f a.out strings strings.o x.c xs.c csh errs
	rm -f ${OBJS}
	rm -rf vgrind

tags:	FRC
	${CTAGS} ${SRCS}

FRC:
