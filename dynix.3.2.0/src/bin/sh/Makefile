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

# $Header: Makefile 2.1 86/02/27 $
#
CFLAGS	= -O -w ${INCLUDE}

OBJS	=\
	args.o blok.o builtin.o cmd.o ctype.o error.o expand.o fault.o io.o\
	macro.o main.o msg.o name.o print.o service.o setbrk.o stak.o string.o\
	word.o xec.o
SED	= sed -f ../../lib/libc/$(MACHINE)/rofix.sed

.c.o:
	$(CC) $(CFLAGS) -SO $< | $(SED) | $(AS) -o $*.o

all:	sh

blok.o:		brkincr.h
fault.o:	brkincr.h
main.o:		brkincr.h
stak.o:		brkincr.h

sh:&	${OBJS}
	${CC} -o sh ${OBJS}

# Handled special to make all data readonly and shared
msg.o ctype.o:
	$(CC) $(CFLAGS) -R -c $<

install:
	install -s -m 755 sh ${DESTDIR}/bin

clean:
	rm -f sh ${OBJS}
