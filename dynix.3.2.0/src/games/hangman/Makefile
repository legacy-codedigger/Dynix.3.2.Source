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
OBJS=	endgame.o extern.o getguess.o getword.o main.o playgame.o \
	prdata.o prman.o prword.o setup.o
CFILES=	endgame.c extern.c getguess.c getword.c main.c playgame.c \
	prdata.c prman.c prword.c setup.c
HEADRS=	hangman.h
#

all:	hangman

tags:	$(HEADRS) $(CFILES)
	ctags $(HEADRS) $(CFILES)

install:
	install -s hangman $(DESTDIR)/usr/games/hangman
	
hangman: $(OBJS)
	$(CC) $(LDFLAGS) -o hangman $(OBJS) -lcurses -ltermlib

clean:
	rm -f $(OBJS) hangman ? core
