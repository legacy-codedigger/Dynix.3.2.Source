
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
NROFFLAG = -DNROFF
CFILES=n1.c n2.c n3.c n4.c n5.c n6.c n7.c n8.c n9.c n10.c ni.c nii.c hytab.c suftab.c
HFILES=../tdef.h ../ext.h tw.h
NFILES=n1.o n2.o n3.o n4.o n5.o n6.o n7.o n8.o n9.o n10.o ni.o nii.o hytab.o suftab.o
INSDIR = $(DESTDIR)/usr/bin

all:	nroff terms

nroff:&	$(NFILES)
	$(CC) $(CFLAGS) -o nroff $(NFILES) 

n1.o:	../n1.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n1.c
n2.o:	../n2.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n2.c
n3.o:	../n3.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n3.c
n4.o:	../n4.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n4.c
n5.o:	../n5.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(USG) $(INCORE) $(NROFFLAG) -c ../n5.c
n6.o:	n6.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I../ -c n6.c
n7.o:	../n7.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n7.c
n8.o:	../n8.c ../ext.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -c ../n8.c
n9.o:	../n9.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n9.c
n10.o:	n10.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I../ -c n10.c
ni.o:	../ni.c ../tdef.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -c ../ni.c
nii.o:	../nii.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../nii.c
hytab.o:	../hytab.c
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -c ../hytab.c
suftab.o:	../suftab.c
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -c ../suftab.c

terms:
	cd terms.d;  make $(MFLAGS) $(MRULES)

install:
	install -c -m 755 nroff $(INSDIR)/nroff
	cd terms.d;  make $(MFLAGS) $(MRULES) install

clean clobber:
	rm -f $(NFILES) nroff
	cd terms.d;  make $(MFLAGS) $(MRULES) clean
