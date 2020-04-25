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

#ident "$Header: Makefile 1.1 1991/04/12 22:51:40 $"

#  crash compress/uncompress makefile
COMFLAGS=   -O -DBSD4_2 $(INCLUDE)
ETC=	$(DESTDIR)/usr/etc


all :&	ccompress unccompress
ccompress:	compress.c main.c
	$(CC) $(COMFLAGS) -o ccompress compress.c main.c


unccompress:	uncompress.c 
	$(CC) $(COMFLAGS) -o unccompress uncompress.c


install:
	install -s ccompress $(ETC)/ccompress
	install -s unccompress $(ETC)/unccompress

clean:
	rm -f ccompress unccompress a.out core *.o
