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

all:	spell hlist hlista hlistb hstop spellin spellout

spell:	spell.o
	${CC} -o spell spell.o

## Run on host so we must use 'cc' ##
xspellin: spellin.c
	cc -O -o xspellin spellin.c

spellout: spellout.o
	${CC} -o spellout spellout.o

spellin: spellin.o
	${CC} -o spellin spellin.o

hlist:	xspellin words
	xspellin < words > hlist

hlista: american local hlist xspellin
	(cat american local)|xspellin hlist > hlista

hlistb: british local hlist xspellin
	(cat british local)|xspellin hlist > hlistb

hstop: stop xspellin
	xspellin < stop > hstop

install:
	install -s spell ${DESTDIR}/usr/lib
	install -s spellin ${DESTDIR}/usr/bin
	install -s spellout ${DESTDIR}/usr/bin
	install -c hlista ${DESTDIR}/usr/dict
	install -c hlistb ${DESTDIR}/usr/dict
	install -c hstop ${DESTDIR}/usr/dict
	install -c spell.sh $(DESTDIR)/usr/bin/spell
	rm -f ${DESTDIR}/usr/lib/w2006
	rm -f ${DESTDIR}/usr/dict/words
	install -c -m 644 words ${DESTDIR}/usr/dict/words
	ln -s /usr/dict/words ${DESTDIR}/usr/lib/w2006

clean:	
	rm -f *.o xspellin spell spellin hlista hlistb hlist hstop spellout core errs
