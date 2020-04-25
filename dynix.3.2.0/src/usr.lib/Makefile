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

# $Header: Makefile 2.18 1991/10/04 20:48:30 $
#
# lpr and sendmail moved to src/ucb
CFLAGS	= -O $(INCLUDE) $(DEBUG)

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR	= learn lib2648 libcurses libdbm libln libm libmp libplot\
	  libtermlib libg libseq/$(MACHINE) libsanity libpc librpcsvc liby \
	  libreadline libutil

# These directories are dropped from VAX 4.2BSD
#
NEVER	= libpc libF77 libI77 libU77 libnm

# Shell scripts that need only be installed and are never removed.
#
SCRIPT	= makewhatis configlibf

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD	= atrun getNAME makekey

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	

all		:& $(SUBDIR) libpps

pgms		:& $(STD) $(NSTD)
$(SUBDIR)	:  FRC; cd $@ && make $(MFLAGS) $(MRULES)

# libpps is not included on the utilities source distribution

libpps		:  FRC
		-sh -c 'if [ -d libpps ] ;then \
			cd libpps; \
			make $(MFLAGS) $(MRULES); \
		fi'

$(STD)		:; $(CC) $(CFLAGS) -o $@ $@.c
atrun 		: atrun.c
getNAME		: getNAME.c
makekey		: makekey.c
pinstall	: FRC; install -s $(STD) $(NSTD) $(DESTDIR)/usr/lib
install		:; for i in $(SUBDIR); do \
			(cd $$i; make $(MFLAGS) $(MRULES) install); done
		   -sh -c 'if [ -d libpps ] ;then \
			cd libpps; \
			make $(MFLAGS) $(MRULES) install; \
		   fi'
		   for i in $(SCRIPT); do \
			(install -c -m 755 $$i.sh $(DESTDIR)/usr/lib/$$i); done
		   install -c -m 644 lib.b $(DESTDIR)/usr/lib
FRC		:
clean		:; for i in $(SUBDIR); do \
			(cd $$i; make $(MFLAGS) $(MRULES) clean); done
		   rm -f $(STD) $(NSTD)
