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

# $Header: /usr/src/dynix.3.2.0/src/usr.etc/RCS/Makefile,v 1.2 1993/03/29 13:09:36 bruce Exp $
#
CFLAGS=	-O $(INCLUDE) $(DEBUG)
RPCLIB=	-lrpcsvc
#

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	crash config emon ftpd monitor routed rwhod sweepd termcap tftpd \
	talkd named rshd rlogind ocheck rmvtoc fsdb timed telnetd bootpd \
	tcp_trace savecore ccompress snmpd

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	ac accton catman comsat dumpfs edquota fingerd inetd mkproto \
	noage nopff noswap quotacheck quotaon repquota rexecd \
	rpcinfo	sa swapon ttyconfig tunefs arp

# Programs that must run setuid to root
#
SETUID=	ping

# Programs that must run setgid to daemon(=10)
#
SETGID=	quot  sestat

# C programs that live in the current directory and need explicit make lines.
#
NSTD= 

# C programs that use the rpcsvc library
#
RPCSVC=	rpc.rusersd rpc.rwalld rpc.sprayd \
	rpc.rstatd rpc.rquotad rwall spray

all	:& $(STD) $(SETUID) $(SETGID) $(NSTD) $(RPCSVC) $(SUBDIR)

$(SUBDIR): FRC
	cd $@ && make $(MFLAGS) $(MRULES)

$(STD) $(SETUID) $(SETGID):
	$(CC) $(CFLAGS) -o $@ $@.c

$(RPCSVC):
	$(CC) $(CFLAGS) -o $@ $@.c $(RPCLIB)

install:
#	for i in $(SCRIPT); do \
#		install -c $$i.sh $(DESTDIR)/usr/etc/$$i; done
	for i in $(STD) $(NSTD) $(RPCSVC); do \
		install -s $$i $(DESTDIR)/usr/etc/$$i; done
	ln $(DESTDIR)/usr/etc/quotaon $(DESTDIR)/usr/etc/quotaoff
	ln -s ../usr/etc/inetd $(DESTDIR)/etc/inetd
	for i in $(SETUID); do \
		install -s -m 4755 $$i $(DESTDIR)/usr/etc/$$i; done
	for i in $(SETGID); do \
		install -s -m 2755 -g daemon $$i $(DESTDIR)/usr/etc/$$i; done
	for i in $(SUBDIR); do \
		(cd $$i && make $(MFLAGS) $(MRULES) install); done

clean:
	rm -f $(STD) $(SETUID) $(SETGID) $(NSTD) $(RPCSVC) *.o
	for i in $(SUBDIR); do \
		(cd $$i && make $(MFLAGS) $(MRULES) clean); done

FRC:

dumpfs		:	dumpfs.c
edquota		:	edquota.c
inetd		:	inetd.c
fingerd		:	fingerd.c
ping		:	ping.c
arp		:	arp.c
quot		:	quot.c
rpc.rusersd	:	rpc.rusersd.c
rpc.rwalld	:	rpc.rwalld.c
rpc.sprayd	:	rpc.sprayd.c
rpc.rstatd	:	rpc.rstatd.c
rpc.rquotad	:	rpc.rquotad.c
rpcinfo		:	rpcinfo.c
rwall		:	rwall.c
spray		:	spray.c
tunefs		:	tunefs.c
ac		:	ac.c
accton		:	accton.c
catman		:	catman.c
comsat		:	comsat.c
sa		:	sa.c
swapon		:	swapon.c
noage		:	noage.c
nopff		:	nopff.c
noswap		:	noswap.c
quotacheck	:	quotacheck.c
quotaon		:	quotaon.c
repquota	:	repquota.c
sestat		:	sestat.c
rexecd		:	rexecd.c
