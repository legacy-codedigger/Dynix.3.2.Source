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
CFLAGS=	-O ${INCLUDE}
#
OBJS= 	gcd.o madd.o mdiv.o mout.o msqrt.o mult.o pow.o util.o

.c.o:
	$(CC) -p -DPROF $(CFLAGS) -c $*.c; \
	$(LD) -X -r $*.o -o profiled/$*.o; \
	$(CC)    -UPROF $(CFLAGS) -c $*.c; \
	$(LD) -x -r $*.o -o $*.O; mv $*.O $*.o

all:	profiled libmp.a libmp_p.a

libmp.a libmp_p.a:& ${OBJS}
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 *.o profiled/*.o
	@-chgrp daemon *.o profiled/*.o > /dev/null 2>&1
	@-chown  root *.o profiled/*.o > /dev/null 2>&1
	cd profiled; ${AR} cru ../libmp_p.a ${OBJS}
	${AR} cru libmp.a ${OBJS}

profiled:
	mkdir profiled

install:
	install -m 644 libmp_p.a ${DESTDIR}/usr/lib/libmp_p.a
	${RANLIB} ${DESTDIR}/usr/lib/libmp_p.a
	install -m 644 libmp.a ${DESTDIR}/usr/lib/libmp.a
	${RANLIB} ${DESTDIR}/usr/lib/libmp.a

clean:
	rm -fr *.o libmp.a libmp_p.a core a.out errs profiled
