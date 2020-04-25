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
# ddt makefile
#
# conditional compilation flags:
#	DEBUG	print various things for debugging ddt

OPT	= -O
#OPT	= -DDEBUG
CFLAGS 	= ${OPT} ${INCLUDE} ${CEFLAGS}

# ----------------------------------------------------------------------
# files
#
SRCS	= main.c display.c parse.c sym.c scan.c mem.c reg.c \
	process.c bpt.c stops.c call.c machine.c bsearch.c

HEADRS 	= main.h error.h scan.h parse.h sym.h display.h bpt.h \
	machine.h fpu.h

OBJS 	= main.o display.o sym.o scan.o parse.o mem.o \
	reg.o process.o bpt.o stops.o call.o machine.o bsearch.o

# ----------------------------------------------------------------------
# building rules  
#
all:	ddt

ddt:&	${OBJS}
	${CC} -o ddt ${OBJS} -lseq

# ----------------------------------------------------------------------
# misc rules
#

install:
	install -s ddt ${DESTDIR}/bin/ddt

clean:
	rm -f ${OBJS} ddt

print:
	pr ${HEADRS} ${SRCS} | lpr

tags:
	ctags ${SRCS}

# ----------------------------------------------------------------------
# dependency rules
#
depend:
	grep '^#[ 	]*include' ${SRCS} \
		| sed '/<.*>/d' \
		| sed 's/:[^"]*"\([^"]*\)".*/: \1/' \
		| sed 's/\.c/.o/' >makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it


