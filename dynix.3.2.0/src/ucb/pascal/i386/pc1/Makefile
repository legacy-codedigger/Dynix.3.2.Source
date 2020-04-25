# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.3 1991/08/30 21:23:54 $

DEST = ${DESTDIR}/usr/lib

I=/usr/include
S=/usr/include/sys
P=

DEBUG=	-g
CDEFS=	-DKBUG
CFLAGS= ${DEBUG} ${CDEFS} ${INCLUDE}

HDR=	fort.h mac2defs.h macdefs.h config.h manifest.h ndu.h pass2.h
SRC=	fort.c common.c allo.c match.c reader.c local2.c order.c table.c
OBJ=	fort.o common.o allo.o match.o reader.o local2.o order.o table.o
GENh=	pcclocal.h
GENc=	
GEN=	${GENh} ${GENc}
SOURCE= Makefile ${SRC} ${HDR}


all: pc1

pc1:$P ${OBJ} 
	${CC} ${DEBUG} ${OBJ} -o pc1 -h

table.o:
	${CC} -c -R ${CFLAGS} table.c

pcclocal.h: ../../../../include/pcc.h
	rm -f pcclocal.h
	egrep '^#[ 	]*(define[ 	][ 	]*PCC(F|T|TM|OM)?_|ifdef|ifndef|endif|else)' ../../../../include/pcc.h | sed -e 's/PCC[A-Z]*_//' > pcclocal.h 

clean: FRC
	rm -f Makefile.bak pcclocal.h *.o pc1 tmp a.out core tags

depend:	${HDR} ${SRC} ${GEN} FRC
	maketd -a ${CDEFS} ${INCLUDE} ${SRC} ${GENc}

install: all FRC
	install -c -s pc1 ${DEST}

lint: ${HDR} ${SRC} ${GEN} FRC
	lint -abphnv ${CDEFS} ${INCLUDE} ${SRC} ${GENc}

print:	Makefile pcclocal.h source
	lpr -J"Source for pc1" Makefile pcclocal.h ${HDR} ${SRC}

source:	${SOURCE}
	@echo "Source checked out."

spotless: clean
	rcsclean ${SOURCE}

tags: ${HDR} ${SRC}
	ctags -t ${HDR} ${SRC}

${SOURCE}:
	co $@

FRC:
	

# DO NOT DELETE THIS LINE - maketd DEPENDS ON IT

fort.o: $I/stdio.h config.h fort.c fort.h mac2defs.h macdefs.h manifest.h \
	ndu.h pass2.h pcclocal.h

common.o: $I/stdio.h common.c config.h mac2defs.h macdefs.h manifest.h ndu.h \
	pass2.h pcclocal.h

allo.o: $I/stdio.h allo.c config.h mac2defs.h macdefs.h manifest.h ndu.h \
	pass2.h pcclocal.h

match.o: $I/ctype.h $I/stdio.h config.h mac2defs.h macdefs.h manifest.h \
	match.c ndu.h pass2.h pcclocal.h

reader.o: $I/fcntl.h $I/stdio.h $S/types.h config.h mac2defs.h macdefs.h \
	manifest.h ndu.h pass2.h pcclocal.h reader.c

local2.o: $I/ctype.h $I/stdio.h config.h local2.c mac2defs.h macdefs.h \
	manifest.h ndu.h pass2.h pcclocal.h

order.o: $I/stdio.h config.h mac2defs.h macdefs.h manifest.h ndu.h order.c \
	pass2.h pcclocal.h

table.o: $I/stdio.h config.h mac2defs.h macdefs.h manifest.h ndu.h pass2.h \
	pcclocal.h table.c

# *** Do not add anything here - It will go away. ***
