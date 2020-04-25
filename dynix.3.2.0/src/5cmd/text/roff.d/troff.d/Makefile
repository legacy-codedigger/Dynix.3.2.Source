
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

INCORE = -DINCORE
USG = -DUSG
CFILES=n1.c n2.c n3.c n4.c n5.c t6.c n7.c n8.c n9.c t10.c ni.c nii.c hytab.c suftab.c
HFILES=../tdef.h ../ext.h dev.h
TFILES=n1.o n2.o n3.o n4.o n5.o t6.o n7.o n8.o n9.o t10.o ni.o nii.o hytab.o suftab.o
INSDIR = $(DESTDIR)/usr/bin

all:	troff fonts

troff:&	$(TFILES)
	$(CC) $(CFLAGS) -o troff $(TFILES) 

n1.o:	../n1.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n1.c
n2.o:	../n2.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n2.c
n3.o:	../n3.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n3.c
n4.o:	../n4.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n4.c
n5.o:	../n5.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(USG) $(INCORE) -c ../n5.c
t6.o:	t6.c ../tdef.h dev.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -I../ -c t6.c
n7.o:	../n7.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n7.c
n8.o:	../n8.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n8.c
n9.o:	../n9.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n9.c
t10.o:	t10.c ../tdef.h dev.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -I../ -c t10.c
ni.o:	../ni.c ../tdef.h
	$(CC) $(CFLAGS) $(INCORE) -c ../ni.c
nii.o:	../nii.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../nii.c
hytab.o:	../hytab.c
	$(CC) $(CFLAGS) $(INCORE) -c ../hytab.c
suftab.o:	../suftab.c
	$(CC) $(CFLAGS) $(INCORE) -c ../suftab.c

fonts:	tc hc ta i10
NOTYET= aps

tc:	tc.o draw.o
	$(CC) $(CFLAGS) -o tc tc.o draw.o -lm

hc:	hc.o draw.o
	$(CC) $(CFLAGS) -o hc hc.o draw.o -lm

ta:	ta.o draw.o
	$(CC) $(CFLAGS) -o ta ta.o draw.o -lm

tc.o:	dev.h
hc.o:	dev.h
ta.o:	dev.h

aps:	draw.o makedev
	cd devaps;  make $(MFLAGS) $(MRULES)

i10:	draw.o makedev
	cd devi10;  make $(MFLAGS) $(MRULES)

makedev:	makedev.c dev.h
	cc $(CFLAGS) -o makedev makedev.c

install:
	install -c -m 755 troff $(INSDIR)/troff
	install -c -m 755 tc $(INSDIR)/tc
	install -c -m 755 hc $(INSDIR)/hc
	install -c -m 755 ta $(INSDIR)/ta
	cd devi10; make $(MFLAGS) $(MRULES) install

clean clobber:
	rm -f $(TFILES) draw.o hc.o ta.o tc.o troff hc ta tc makedev
	cd devi10;  make $(MFLAGS) $(MRULES) clean
