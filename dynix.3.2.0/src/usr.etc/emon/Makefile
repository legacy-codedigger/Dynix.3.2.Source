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

# $Header: Makefile 2.4 87/04/11 $

# $Log:	Makefile,v $

MIPCOPTS=-I. -I../h ${INCLUDE} ${IDENT}

COPTS=	 ${MIPCOPTS}

CFLAGS=	-O $(INCLUDE)

CFILES=	emon_main.c emon_cmds.c emon_filter.c emon_ttab.c\
	emon_globals.c emon_lookit.c emon_trap.c \
	emon_test.c emon_in_cksum.c printsmb.c

HFILES=	emon.h ll.h

OBJS=	emon_main.o emon_cmds.o emon_filter.o emon_ttab.o\
	emon_globals.o emon_lookit.o emon_trap.o printsmb.o \
	emon_test.o emon_in_cksum.o

all:&	gopro emon 

gopro:	gopro.o
	${CC} -o gopro gopro.o

emon:&	${OBJS} Makefile
	${CC} -o emon ${OBJS}

print:
	pr ${CFILES} ${HFILES} ${ASFILES} | lpr

clean:
	rm -f *.o emon gopro

tags:	${CFILES} ${HFILES} ${ASFILES}
	${CTAGS} ${CFILES} ${HFILES} ${ASFILES}

install:; install -s emon gopro $(DESTDIR)/usr/etc

lint:
	${LINT} -hbxn ${CPUDEF} ${MIPCOPTS} ${PARAM} ${CFILES} \
		| sed -f ../debug/lint.sed

depend:
	echo '# DO NOT DELETE THIS LINE  --  make depend uses it' > makedep
	$(CC) -M $(CFLAGS) $(CFILES) | \
	sed -e ':loop' \
	    -e 's/\.\.\/[^ /]*\/\.\./../' \
	    -e 't loop'  -e 's/\.\///g' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' >> makedep
	echo '/^# DO NOT DELETE THIS LINE/,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ex - Makefile < eddep
	rm eddep makedep

# DO NOT DELETE THIS LINE  --  make depend uses it
