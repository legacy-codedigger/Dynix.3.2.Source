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
CFLAGS=	-O -DCM_N -DCM_GT -DCM_B -DCM_D ${INCLUDE}
#
OBJS=	termcap.o tgoto.o tputs.o

.c.o:
	$(CC) -p -DPROF $(CFLAGS) -c $*.c; \
	$(LD) -X -r $*.o -o profiled/$*.o; \
	$(CC)    -UPROF $(CFLAGS) -c $*.c; \
	$(LD) -x -r $*.o -o $*.O; mv $*.O $*.o

all:	profiled termcap.a termcap_p.a

termcap.a termcap_p.a:& ${OBJS}
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 *.o profiled/*.o
	@-chgrp daemon *.o profiled/*.o > /dev/null 2>&1
	@-chown  root *.o profiled/*.o > /dev/null 2>&1
	cd profiled; ${AR} cru ../termcap_p.a ${OBJS}
	${AR} cru termcap.a ${OBJS}

profiled:
	mkdir profiled

install:
	install -c -m 644 termcap.a ${DESTDIR}/usr/lib/libtermcap.a
	rm -f ${DESTDIR}/usr/lib/libtermlib.a
	ln ${DESTDIR}/usr/lib/libtermcap.a ${DESTDIR}/usr/lib/libtermlib.a
	${RANLIB} ${DESTDIR}/usr/lib/libtermcap.a
	install -c -m 644 termcap_p.a ${DESTDIR}/usr/lib/libtermcap_p.a
	rm -f ${DESTDIR}/usr/lib/libtermlib_p.a
	ln ${DESTDIR}/usr/lib/libtermcap_p.a ${DESTDIR}/usr/lib/libtermlib_p.a
	${RANLIB} ${DESTDIR}/usr/lib/libtermcap_p.a
	-if [ "${EXPORT}" != TRUE ]; then \
		install -c -m 644 termcap.a ${CRYPT_DESTDIR}/usr/lib/libtermcap.a; \
		rm -f ${CRYPT_DESTDIR}/usr/lib/libtermlib.a; \
		ln ${CRYPT_DESTDIR}/usr/lib/libtermcap.a ${CRYPT_DESTDIR}/usr/lib/libtermlib.a; \
		${RANLIB} ${CRYPT_DESTDIR}/usr/lib/libtermcap.a; \
	fi

clean:
	rm -fr termcap.a termcap_p.a ${OBJS} core a.out errs profiled
