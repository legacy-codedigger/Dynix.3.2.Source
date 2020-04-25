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

# $Header: Makefile 1.2 89/03/12 $

SRCDIR		= ../src
CFLAGS		= -O -I$(SRCDIR) $(INCLUDE) -DNOPC2
FILES		= pcexterns.o how_pc how_pi how_pix how_pxp

all:	pc pc3 pmerge pcexterns.o

install:
	install -s pc pmerge $(DESTDIR)/usr/ucb
	install -c -m 644 $(FILES) $(DESTDIR)/usr/lib
	install -s pc3 $(DESTDIR)/usr/lib/pc3
	@echo "pxref is STUBBED"
#	-pc -w pxref.p
#	install -s a.out $(INSTALL)/pxref

pmerge: pmerge.c
	$(CC) $(CFLAGS) -o pmerge pmerge.c

pc3:	pc3.c pc3.h $(SRCDIR)/pstab.h
	$(CC) $(CFLAGS) pc3.c -o pc3

pxref:	pxref.p
	pc -w pxref.p -o pxref

pc:	pc.c
	$(CC) $(CFLAGS) -o pc pc.c

LIBPC 	= /usr/lib/libpc.a
LIBC	= /lib/libc.a
LIBM	= /usr/lib/libm.a
LIBG	= /usr/lib/libg.a
D_LIBPC	= $(DESTDIR)$(LIBPC)
D_LIBC	= $(DESTDIR)$(LIBC)
D_LIBM	= $(DESTDIR)$(LIBM)
D_LIBG	= $(DESTDIR)$(LIBG)
LIBRARIES = $(D_LIBPC) $(D_LIBC) $(D_LIBM) $(D_LIBG)

pcexterns.o: pcexterns.awk 
	( echo -n $(LIBPC):; $(NM) $(D_LIBPC) ) \
		| awk -f pcexterns.awk \
		| sort -t\" +1 -2 -u >pcexterns.s
	( echo -n $(LIBC):; $(NM) $(D_LIBC) ) \
		| awk -f pcexterns.awk \
		| sort -t\" +1 -2 -u >>pcexterns.s
	( echo -n $(LIBM):; $(NM) $(D_LIBM) ) \
		| awk -f pcexterns.awk \
		| sort -t\" +1 -2 -u >>pcexterns.s
	( echo -n $(LIBG):; $(NM) $(D_LIBG) ) \
		| awk -f pcexterns.awk \
		| sort -t\" +1 -2 -u >>pcexterns.s
	$(AS) pcexterns.s -o pcexterns.o
	rm -f pcexterns.s

opcode.h: $(SRCDIR)/OPnames.h $(SRCDIR)/opc.c
	rm -f opcode.h
	cc $(SRCDIR)/opc.c -o opc
	opc >opcode.h
	rm -f opc

picture: $(SRCDIR)/OPnames.h $(SRCDIR)/pic.c
	rm -f picture
	cc $(SRCDIR)/pic.c -o pic
	pic >picture
	rm -f pic

clean:
	rm -f pix px_header pxref pmerge pc pc3 \
	pcexterns.s *.o errs opc opc.c pic pic.c picture

print: picture
	@pr READ_ME makefile picture
	@ls -l | pr
	@pr pix.c pmerge.c pxref.p pc.c pc3.h pc3.c

FRC:
