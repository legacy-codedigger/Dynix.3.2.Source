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
#

# $Header: Makefile 1.2 87/05/11 $
#
# Top level Makefile that builds the entire NFS utility and command set
#
# Programs that live in subdirectories, and have makefiles of their own.
#

INSTALL= etc.i usr.bin.i usr.etc.i
SUBDIR=  etc usr.bin usr.etc doc
CLEAN = doc.k etc.k usr.bin.k usr.etc.k

all		:; @echo "this makefile not meant to be invoked directly"; exit 1

subdirs		:& $(SUBDIR)

$(SUBDIR)	: FRC; cd $@ && \
			time make $(MFLAGS) $(MRULES) all \
			>>../../makelog.nfs.$@ 2>&1

dist		: FRC; cd $@ && \
			time make $(MFLAGS) $(MRULES) all install \
			>>../../makelog.nfs.$@ 2>&1

# These are stubs for now, to cleanly support addition of NFS in up-level
# makefile.
headers \
libraries	:

nfs.cmds	:& $(INSTALL)

$(INSTALL) \
doc.i		:; -cd `basename $@ .i` && \
			time make $(MFLAGS) $(MRULES) all install \
			>>../../makelog.nfs.`basename $@ .i` 2>&1 && touch ../$@

clean		:& $(CLEAN); rm -f a.out core *.s *.o *.i

$(CLEAN)	:; cd `basename $@ .k` && make $(MFLAGS) $(MRULES) clean

FRC		:
