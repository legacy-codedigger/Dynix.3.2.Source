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

# $Header: Makefile 2.0 86/01/28 $
#
# Curses package
#
CFLAGS=	-O ${INCLUDE}
#
CTAGS=	ctags
OBJS=	addch.o addstr.o box.o clear.o clrtobot.o clrtoeol.o cr_put.o \
	cr_tty.o curses.o delch.o deleteln.o delwin.o endwin.o erase.o \
	getch.o getstr.o initscr.o insch.o insertln.o longname.o move.o \
	mvprintw.o mvscanw.o mvwin.o newwin.o overlay.o overwrite.o \
	printw.o refresh.o scanw.o scroll.o standout.o touchwin.o tstp.o \
	unctrl.o

.c.o:
	$(CC) -p -DPROF $(CFLAGS) -c $*.c; \
	$(LD) -X -r $*.o -o profiled/$*.o; \
	$(CC)    -UPROF $(CFLAGS) -c $*.c; \
	$(LD) -x -r $*.o -o $*.O; mv $*.O $*.o

all:	profiled libcurses libcurses_p

libcurses libcurses_p:& ${OBJS}
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 *.o profiled/*.o
	@-chgrp daemon *.o profiled/*.o > /dev/null 2>&1
	@-chown  root *.o profiled/*.o > /dev/null 2>&1
	@echo building profiled libcurses
	@cd profiled; ${AR} cr ../libcurses_p ${OBJS}
	@echo building normal libcurses
	@${AR} cr libcurses ${OBJS}

profiled:
	mkdir profiled

install:
	install -m 644 libcurses ${DESTDIR}/usr/lib/libcurses.a
	${RANLIB} ${DESTDIR}/usr/lib/libcurses.a
	install -m 644 libcurses_p ${DESTDIR}/usr/lib/libcurses_p.a
	${RANLIB} ${DESTDIR}/usr/lib/libcurses_p.a

clean:
	rm -fr ${OBJS} a.out core test errs libcurses libcurses_p profiled

ctags:
	${CTAGS} ${CFILES} ${DESTDIR}/usr/include/curses.h

lint:
	lint -hxb ${CFILES} -lcurses > lint.out

test:	libcurses test.o
	${CC} ${LDFLAGS} ${CFLAGS} -o test test.o crlib -ltermlib
