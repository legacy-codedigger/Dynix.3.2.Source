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
#
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 2.22 1991/05/14 23:01:51 $
#
# Top level Makefile that builds the entire 4.2 BSD system.
#	and system 5 application environment.
#
# Programs that live in subdirectories, and have makefiles of their own.
#

INSTALL= sys.i ucb.i usr.bin.i bin.i etc.i games.i new.i stand.i usr.etc.i
SUBDIR=  sys   ucb   usr.bin   bin   etc   games   new   stand   usr.etc \
	 5cmd  doc
CLEAN = 5cmd.k 5include.k 5lib.k bin.k dist.k doc.k etc.k games.k include.k \
	lib.k new.k stand.k sys.k ucb.k usr.bin.k usr.lib.k usr.etc.k

all:
	@echo "this makefile not meant to be invoked directly" 
	exit 1

libraries:& dynix.libraries 5lib

subdirs:& $(SUBDIR)

$(SUBDIR): FRC
	cd $@ && time make $(MFLAGS) $(MRULES) all >>../../makelog.$@ 2>&1

dynix.libraries: lib.i usr.lib.i

5lib: FRC
	sh -c "if [ -f $@/Makefile ]; then \
			cd $@ && \
	time make $(MFLAGS) $(MRULES) libc libc.install all install \
	>>../../makelog.$@ 2>&1; \
		       fi; exit 0"

dynix.cmds:& ps.SV usr.lib.cmds $(INSTALL) gnu compilersandtools atags

# SystemV ps has dependencies on libc and libseq so do here as a special
ps.SV: FRC
	cd 5cmd && make $(MFLAGS) $(MRULES) ps.install >>../../makelog.5cmd 2>&1

usr.lib.cmds:
	-cd usr.lib && \
			time make $(MFLAGS) $(MRULES) pgms pinstall \
			>>../../makelog.usr.lib 2>&1

compilersandtools:
	-cd ../compilers && \
			time make $(MFLAGS) $(MRULES) all install \
			>>../makelog.compilers 2>&1

atags:
	-sh -c 'if [ -d ../tools.src/a.tags ] ;then \
			   cd ../tools.src/a.tags; \
			   make $(MFLAGS) $(MRULES) all install; \
		   fi'
# atags source is not available in the source distribution

headers:& sys.headers include.headers 5include.headers stand.headers

sys.headers \
include.headers \
5include.headers \
stand.headers	:
	cd `basename $@ .headers` && \
			time make $(MFLAGS) $(MRULES) install.headers \
			>>../../makelog.$@ 2>&1

$(INSTALL) \
doc.i \
dist.i \
lib.i \
usr.lib.i \
5cmd.i:
	-cd `basename $@ .i` && \
			time make $(MFLAGS) $(MRULES) all install \
			>>../../makelog.`basename $@ .i` 2>&1 && touch ../$@

gnu: FRC
	-rm -rf $(DESTDIR)/usr/local/gnu && \
			umask 022 && mkdir $(DESTDIR)/usr/local/gnu && \
			cd public/gnu && \
			tar cfB - dist | \
			(cd $(DESTDIR)/usr/local/gnu && tar xfB -) && \
			cd $(DESTDIR)/usr/local/gnu && \
			find . -name '.ccsmark' -exec rm -f '{}' ';' && \
			find . -exec /etc/chown root '{}' ';' && \
			find . -exec /bin/chgrp daemon '{}' ';'

mirroring:
	for i in $(SUBDIR) ; \
	do if grep -s 'mirroring.*:' $$i/Makefile ; \
		then make $(MFLAGS) $(MRULES) $$i ; \
		fi ; \
	done

mirror_install:
	for i in $(INSTALL) ; \
	do if grep -s 'mirror_install.*:' $$i/Makefile ; \
		then make $(MFLAGS) $(MRULES) $$i ; \
		fi ; \
	done

clean:& $(CLEAN)
	rm -f a.out core *.s *.o *.i

$(CLEAN):
	cd `basename $@ .k` && make $(MFLAGS) $(MRULES) clean

FRC:
