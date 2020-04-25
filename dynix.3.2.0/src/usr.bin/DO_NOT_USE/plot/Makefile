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

ALL=	tek t300 t300s t450 vplot

all:&	${ALL}

${ALL}:	driver.o

tek:
	${CC} -o tek  driver.o -l4014 -lm

t300:
	${CC} -o t300 driver.o -l300 -lm

t300s:
	${CC} -o t300s driver.o -l300s -lm

t450:
	${CC} -o t450 driver.o -l450 -lm

vplot:	vplot.o chrtab.o
	${CC} -o vplot vplot.o chrtab.o

install:
	install -s tek ${DESTDIR}/usr/bin
	install -s t300 ${DESTDIR}/usr/bin
	install -s t300s ${DESTDIR}/usr/bin
	install -s t450 ${DESTDIR}/usr/bin
	install -s vplot ${DESTDIR}/usr/bin
	install -c plot.sh ${DESTDIR}/usr/bin/plot

clean:
	rm -f *.o ${ALL} a.out core errs
