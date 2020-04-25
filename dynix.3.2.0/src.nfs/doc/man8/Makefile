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

# $Header: Makefile 1.11 87/07/29 $
#
.SUFFIXES:
.SUFFIXES: .8 .8c .8s .Z .Zc .Zs .P .Pc .Ps

M	= $(NFS_DESTDIR)/usr/man/cat8
C	= 8
ITPF	= /usr/imagen/bin/itpf -x

# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

PAGES	=\
	makedbm.$C mountd.$Cc nfsd.$C nfsstat.$C showmount.$C ypinit.$C\
	ypmake.$C yppasswdd.$Cc yppoll.$C yppush.$C ypserv.$C ypset.$C\
	ypxfr.$C

# If you wondered what happened to ypwhich.8, it never existed
# according to Anne Ruggles at Sun.

.8.Z .8c.Zc:
	cd ..; umask 111; nroff $(MANMACS) man8/$< | compress > $M/$<.Z
.8.P .8c.Pc .8s.Ps:
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:; install -c -m 444 $(PAGES) $(NFS_DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z pages

pages	:& $(PAGES)

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages
