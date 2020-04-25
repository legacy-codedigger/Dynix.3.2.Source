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

# $Header: Makefile 2.1 86/06/25 $
#
CFLAGS	= -O ${INCLUDE}
WORDS	= ../../usr.bin/spell/words
#

all:	boggle bogdict

bogdict: comp sfile
	sed -f sfile $(WORDS)| uniq | ./comp >bogdict

boggle: boggle.c
	${CC} -o boggle ${CFLAGS} boggle.c

# cc needed here
comp: comp.c
	cc -o comp ${CFLAGS} comp.c

install:
	install -s boggle  ${DESTDIR}/usr/games/boggle
	install -m 444 bogdict ${DESTDIR}/usr/games/bogdict

clean:
	rm -f bogdict boggle comp
