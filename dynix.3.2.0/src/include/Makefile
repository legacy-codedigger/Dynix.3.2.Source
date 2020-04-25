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

# $Header: Makefile 2.17 1991/07/03 16:22:29 $
#
# Doing a make install.headers builds /usr/include
#
# The ``rm -rf''s used below are safe because rm doesn't
# follow symbolic links.
#

SUBDIRS	= arpa protocols pascal
LINKS	= signal.h time.h
UFSLINKS= fs.h fsdir.h inode.h
MACHDEP	= $(MACHINE) mbad sec ssm mirror zdc balance stand stripe
NETDIRS	= net netif netinet netat
NFSDIRS	= rpc nfs ufs
MIRDIRS = mirror
STRDIRS = stripe
SYSDIRS	= $(NETDIRS) $(MACHDEP) $(NFSDIRS)
IDIRS	= $D/usr/include/arpa \
	  $D/usr/include/pascal \
	  $D/usr/include/protocols \
	  $D/usr/include/parallel \
	  $D/usr/include/rpcsvc
D	= $(DESTDIR)
ST	= $(STRIPE_DESTDIR)
M	= $(MIRROR_DESTDIR)

install		: install.headers
install.headers	: symlinks dirs
		  install -c -m 444 *.h \
		  	arpa/* pascal/* protocols/* rpcsvc/* \
			$D/usr/include
# libpps is not included on the utilities source distribution
		  -sh -c 'if [ -d ../usr.lib/libpps ] ;then \
		  	cd ../usr.lib/libpps; \
			make $(MFLAGS) $(MRULES) install.headers; \
		  fi'
		  -sh -c 'if [ -d ../usr.lib/libseq ] ;then \
		  	cd ../usr.lib/libseq/$(MACHINE); \
			make $(MFLAGS) $(MRULES) install.headers; \
		  fi'
		  -for i in $(LINKS); do \
			  rm -f $D/usr/include/$$i && \
			  ln -s sys/$$i $D/usr/include/$$i; \
		  done
		  -for i in $(UFSLINKS); do \
			rm -f $(DESTDIR)/usr/include/sys/$$i; \
			ln -s ../ufs/$$i $D/usr/include/sys/$$i; \
		  done
symlinks	:
		  -for i in $(SYSDIRS); do \
			  rm -rf $D/usr/include/$$i; \
			  ln -s sys/../$$i $D/usr/include/$$i; \
		  done
		  -for i in $(STRDIRS); do \
			  rm -rf $(ST)/usr/include/$$i; \
			  ln -s ../sys/$$i $(ST)/usr/include/$$i; \
		  done
		  -for i in $(MIRDIRS); do \
			  rm -rf $M/usr/include/$$i; \
			  ln -s ../sys/$$i $M/usr/include/$$i; \
		  done
		  rm -rf $D/usr/include/sys
		  rm -rf $D/usr/include/sys
		  ln -s ../../sys/h $D/usr/include/sys
		  rm -f $D/usr/include/machine
		  ln -s $(MACHINE) $D/usr/include/machine
dirs		:& $(IDIRS)
$(IDIRS)	:; umask 022 && mkdir $@
clean		:
