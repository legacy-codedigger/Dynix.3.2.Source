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
SLIBS=	-lm -ltermcap
BIN=	${DESTDIR}/usr/games
LIB=	${DESTDIR}/usr/games/lib
USER=	daemon
OBJS=	snake.o move.o

all:	snake snscore

snake:	$(OBJS)
	${CC} $(OBJS) -o snake $(SLIBS)

snake.o move.o:snake.h

snscore: snscore.c
	${CC} $(CFLAGS) snscore.c -o snscore

busy:	busy.c
	${CC} $(CFLAGS) busy.c -o busy

install:
	install -s -m 4755 -o ${USER} snake ${BIN}/snake
	install -s -m 755 -o ${USER} snscore ${BIN}/snscore
#	install -s -m 755 -o ${USER} busy ${BIN}/busy
	touch ${LIB}/snakerawscores
	-/etc/chown $(USER) $(LIB)/snakerawscores
	chmod 644 $(LIB)/snakerawscores

clean:
	rm -f *.o ${ALL}
