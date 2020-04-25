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

# $Header: Makefile 2.1 86/07/08 $
#
# Sail - Wooden Ships And Iron Men
#
CFLAGS=	-O ${INCLUDE}
#
CTAGS=	ctags -w
DFILES=	driver1.c driver2.c driver3.c grap.c
DOBJS=	driver1.o driver2.o driver3.o grap.o
PFILES=	version.c player1.c player2.c
POBJS=	version.o player1.o player2.o
LFILES=	sail.log.c
LOBJS=	sail.log.o
COMMONFILES= assorted.c boarders.c game.c globals.c\
	main.c parties.c screen.c machdep.c
COMMONOBJS= assorted.o boarders.o game.o globals.o\
	main.o parties.o screen.o machdep.o
HEADRS=	externs.h player.h machdep.h
OTHERFILES= Makefile
CTAGFILES= ${DFILES} ${PFILES} ${LFILES} ${COMMONFILES}
JUNKFILES= sail driver sail.log junk arch tags
PLIBS= 	-lcurses -ltermlib machdep.o
DLIBS=
LLIBS=
SAILMASTER= daemon
SAIL=	sail
DRIVER=	lib/saildriver
SAIL.LOG= sail.log
SHIPSLOG= lib/saillog
TARGET=	${DESTDIR}/usr/games

..c.o:; ${CC} ${CFLAGS} -c $<

all: 	sail driver sail.log

sail:	${COMMONOBJS} ${POBJS}
	${CC} -O ${COMMONOBJS} ${POBJS} ${PLIBS} -o sail

driver: ${COMMONOBJS} ${DOBJS}
	${CC} -O ${COMMONOBJS} ${DOBJS} ${DLIBS} -o driver

${DOBJS} ${POBJS} ${LOBJS} ${COMMONOBJS}: externs.h machdep.h

sail.log: ${LOBJS} globals.o
	${CC} -O ${LOBJS} globals.o -o sail.log

tags: ${CTAGFILES}
	${CTAGS} ${CTAGFILES}

arch:	sail.ar
	${AR} uv sail.ar ${PFILES} ${COMMONFILES} ${OTHERFILES} \
	${DFILES} ${LFILES} ${HEADRS}

driveprofile: ${COMMONOBJS} ${DOBJS}
	${CC} -p ${COMMONOBJS} ${DOBJS} ${CFLAGS} ${DLIBS} -o DRIVE.pro

profile: ${COMMONOBJS} ${POBJS} ${DOBJS}
	${CC} -p ${COMMONOBJS} ${POBJS} ${CFLAGS} ${PLIBS} -o SAIL.pro
	${CC} -p ${COMMONOBJS} ${DOBJS} ${CFLAGS} ${DLIBS} -o DRIVE.pro

clean: 
	rm -f ${DOBJS} ${POBJS} ${LOBJS} ${COMMONOBJS} ${JUNKFILES}

install: ${TARGET}/${SHIPSLOG}
	install -s -m 4711 -o ${SAILMASTER} sail ${TARGET}/${SAIL}
	install -s -m 4711 -o ${SAILMASTER} driver ${TARGET}/${DRIVER}
	install -s -m 711 sail.log ${TARGET}/${SAIL.LOG}

${TARGET}/${SHIPSLOG}:
	cp /dev/null ${TARGET}/${SHIPSLOG}
	-/etc/chown ${SAILMASTER} ${TARGET}/${SHIPSLOG}
	chmod 644 ${TARGET}/${SHIPSLOG}
