# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.9 91/03/04 $

COPTS	= $(INCLUDE) $(DEBUG)
CFLAGS	= -O $(COPTS)
OBJS	= getopt.o misc.o dis.o tables.o usclk.o getusclk.o \
		getscsiinfo.o getzdinfo.o geomtab.o get_vers.o
VPATH	= ../common
C	= ../common
CS	= $(C)/usclk
USCLKE	= c_usclk.c f_usclk.f p_usclk.p makefile
USCLKD	= $(DESTDIR)/usr/sequent/usclk.demo

all:	libseq.a

libseq.a :& $(OBJS)
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod 644 $(OBJS)
	@-chgrp daemon $(OBJS) >/dev/null 2>&1
	@-chown root $(OBJS) >/dev/null 2>&1
	$(AR) cru libseq.a $(OBJS)

usclk.o:	$C/usclk.c
	$(CC) $(CFLAGS) -c usclk.c

getusclk.o:	getusclk.s
	$(CPP) $(COPTS)  $*.s | $(AS) -o $*.o

get_vers.o:	get_vers.s
	$(CPP) -I../../../lib/libc/i386/sys \
		-I../../../include $*.s $(COPTS) | $(AS) -o $*.o

install.headers:
	cd $C; \
	install -c -m 444 usclkc.h usclkp.h usclkf.h $(DESTDIR)/usr/include

install:
	install -m 644 libseq.a $(DESTDIR)/usr/lib/libseq.a
	$(RANLIB) $(DESTDIR)/usr/lib/libseq.a
	-@if [ $(USCLKD) = `pwd` ]; then \
		echo "will not install sources (target directory = current directory)"; \
	else \
		parent=`expr $(USCLKD) : '\(.*\)/'`; \
		if [ ! -d $$parent ]; then \
			echo "	mkdir $$parent; chmod 755 $$parent"; \
			mkdir $$parent; chmod 755 $$parent; \
		fi; \
		if [ ! -d $(USCLKD) ]; then \
			echo "	mkdir $(USCLKD); chmod 755 $(USCLKD)"; \
			mkdir $(USCLKD); chmod 755 $(USCLKD); \
		fi; \
		cd $(CS); \
		echo "	install -c -m 644 $(USCLKE) $(USCLKD)"; \
		install -c -m 644 $(USCLKE) $(USCLKD); \
	fi

clean:
	rm -f libseq.a $(OBJS)

