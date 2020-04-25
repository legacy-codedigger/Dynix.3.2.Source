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

# $Header: Makefile 1.4 90/06/04 $
#
CFLAGS=	-O $(INCLUDE) $(DEBUG)
RPCLIB=	-lrpcsvc
#

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	yp ypserv

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	nfsstat

# C programs that live in the current directory and need explicit make lines.
#
NSTD=

# C programs that use the rpcsvc library
#
RPCSVC=	rpc.mountd rpc.yppasswdd showmount

# C programs (selected from those above) that need to be setgid daemon.
#
SETGID=	nfsstat


all	:& $(STD) $(NSTD) $(RPCSVC) $(SUBDIR)

$(SUBDIR): FRC
	cd $@ && make $(MFLAGS) $(MRULES)

$(STD):
	$(CC) $(CFLAGS) -o $@ $@.c

$(RPCSVC):
	$(CC) $(CFLAGS) -o $@ $@.c $(RPCLIB)

install:
	for i in $(STD) $(NSTD) $(RPCSVC); do \
		install -s $$i $(NFS_DESTDIR)/usr/etc/$$i; done
	for i in $(SUBDIR); do \
		(cd $$i && make $(MFLAGS) $(MRULES) install); done
	for i in $(SETGID); do \
		chgrp daemon $(NFS_DESTDIR)/usr/etc/$$i; \
		chmod g+s $(NFS_DESTDIR)/usr/etc/$$i; \
	done

clean:
	rm -f $(STD) $(NSTD) $(RPCSVC) *.o
	for i in $(SUBDIR); do \
		(cd $$i && make $(MFLAGS) $(MRULES) clean); done

FRC:

nfsstat		:	nfsstat.c
rpc.mountd	:	rpc.mountd.c
rpc.yppasswdd	:	rpc.yppasswdd.c
showmount	:	showmount.c
