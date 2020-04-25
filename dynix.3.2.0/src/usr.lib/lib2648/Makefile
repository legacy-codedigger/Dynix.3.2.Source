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

# $Header: Makefile 2.1 87/04/28 $
#
CFLAGS=	-O -DTRACE ${INCLUDE}
#
OBJS=	agoto.o aminmax.o aon.o areaclear.o beep.o bitcopy.o cleara.o \
	clearg.o curon.o dispmsg.o draw.o drawbox.o dumpmat.o \
	emptyrow.o error.o escseq.o gdefault.o gon.o kon.o line.o mat.o \
	message.o minmax.o motion.o move.o movecurs.o newmat.o outchar.o \
	outstr.o printg.o rawchar.o rbon.o rdchar.o readline.o set.o \
	setmat.o sync.o texton.o ttyinit.o update.o video.o zermat.o \
	zoomn.o zoomon.o zoomout.o

.c.o:
	$(CC) -p -DPROF $(CFLAGS) -c $*.c; \
	$(LD) -X -r $*.o -o profiled/$*.o; \
	$(CC)    -UPROF $(CFLAGS) -c $*.c; \
	$(LD) -x -r $*.o -o $*.O; mv $*.O $*.o

all:	profiled lib2648.a lib2648_p.a

lib2648.a lib2648_p.a:&	${OBJS}
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 *.o profiled/*.o
	@-chgrp daemon *.o profiled/*.o > /dev/null 2>&1
	@-chown  root *.o profiled/*.o > /dev/null 2>&1
	cd profiled; ${AR} cru ../lib2648_p.a ${OBJS}
	${AR} cru lib2648.a ${OBJS}

profiled:
	mkdir profiled

install:
	install -m 644 lib2648.a ${DESTDIR}/usr/lib/lib2648.a
	${RANLIB} ${DESTDIR}/usr/lib/lib2648.a
	install -m 644 lib2648_p.a ${DESTDIR}/usr/lib/lib2648_p.a
	${RANLIB} ${DESTDIR}/usr/lib/lib2648_p.a

clean:
	rm -fr lib2648.a lib2648_p.a ${OBJS} core errs profiled
