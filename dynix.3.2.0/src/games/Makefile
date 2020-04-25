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

CFLAGS=	-O ${INCLUDE}


# Programs that have fallen by the way-side (sigh)

NEVER= zork aard ddl mpu compat ching chess chess.bin

# Programs that live in subdirectories, and have makefiles of their own.
SUBDIR=	backgammon boggle cribbage fortune hangman quiz sail snake trek
NOTYET= mille monop

# These programs can not be build yet.
# Doctor needs lisp, and adventure must be built on target
NOTYET=	doctor adventure

# Shell scripts that need only be installed and are never removed.
#
#SCRIPT=

# C programs that live in the current directory and do not need
# explicit make lines.
STD=	arithmetic banner bcd cfscores factor fish number wump
NOTYET=	btlgammon

# C programs that live in the current directory and need explicit make lines.
NSTD=	canfield primes rain worm worms

all:&	${SUBDIR} rogue ${STD} ${NSTD}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} ${MRULES}

rogue: FRC
	sh -c "if [ -f $@/Makefile ]; then \
		cd $@; make ${MFLAGS} ${MRULES}; \
	else true; fi"

FRC:

${STD}:
	${CC} ${CFLAGS} -o $@ $@.c

install:
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} install); \
	done
	sh -c "if [ -f rogue/Makefile ]; then \
		cd rogue; make ${MFLAGS} ${MRULES} install; \
	else true; fi"
#	for i in ${SCRIPT}; do (install -c $$i ${DESTDIR}/usr/games/$$i); done
	for i in ${STD} ${NSTD}; do \
		(install -s $$i ${DESTDIR}/usr/games/$$i); \
	done

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} clean); \
	done
	sh -c "if [ -f rogue/Makefile ]; then \
		cd rogue; make ${MFLAGS} ${MRULES} clean; \
	else true; fi"
	rm -f ${STD} ${NSTD}

depend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# Files listed in ${NSTD} have explicit make lines given below.

canfield:
	${CC} -o canfield ${CFLAGS} canfield.c -lcurses -ltermcap

primes:
	${CC} -o primes ${CFLAGS} primes.c -lm

rain:
	${CC} -o rain ${CFLAGS} rain.c -lcurses -ltermcap

worm:
	${CC} -o worm ${CFLAGS} worm.c -lcurses -ltermcap

worms:
	${CC} -o worms ${CFLAGS} worms.c -lcurses -ltermcap

# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
