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

# $Header: /usr/src/dynix.3.2.0/src/etc/RCS/Makefile,v 1.2 1993/03/29 13:14:16 bruce Exp $
#
CFLAGS= -O $(INCLUDE)
#

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	dump newfs fsck getty htable restore version mkvtoc prtvtoc fsstat \
	format syslogd showprocs stripe

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	fastboot fasthalt mklost+found

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	addbad bad144 badsect bootflags bp chown \
	clri cron diskpart fsirand gettable \
	ifconfig init mknod \
	on online dmesg pmap portmap pstat \
	renice rmt route showcfg update vipw vmtune

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	usclk_conf reboot halt shutdown

# Kernel profiling support programs, not part of the binary product.
#
KPRO=	prfdc prfld prfpr prfsnap prfstat

# C programs that user the rpcsvc library
#
RPCSVC=	mount umount

# Mirroring layered product
#
# Mirroring programs that need explicit make lines
MIRROR_NSTD= mirror
# Mirroring programs  that don't
MIRROR_STD= mrinval unmirror
# Mirror programs all together
MIRROR= $(MIRROR_STD) $(MIRROR_NSTD)

all:&	$(SUBDIR) $(STD) $(NSTD) $(RPCSVC) mirroring

$(SUBDIR): FRC
	cd $@ && make $(MFLAGS) $(MRULES)

FRC:

mirroring:&	$(MIRROR)

kpro:&	$(KPRO)

$(STD) $(KPRO) x:
	$(CC) $(CFLAGS) -o $@ $@.c

$(MIRROR_STD): 
	$(CC) $(MIRROR_INCLUDE) $(CFLAGS) -o $@ $@.c

$(RPCSVC):
	$(CC) $(CFLAGS) -o $@ $@.c -lrpcsvc

install:	mirror_install
	for i in $(SUBDIR); do \
		(cd $$i; make $(MFLAGS) $(MRULES) install); done
	for i in $(SCRIPT); do \
		(install -c $$i.sh $(DESTDIR)/etc/$$i); done
	install -s $(STD) $(NSTD) $(RPCSVC) $(DESTDIR)/etc
	chmod 744 $(DESTDIR)/etc/init
	chmod 744 $(DESTDIR)/etc/update
	chmod 744 $(DESTDIR)/etc/getty
	chmod 744 $(DESTDIR)/etc/cron
	rm -f $(DESTDIR)/etc/offline
	ln $(DESTDIR)/etc/online $(DESTDIR)/etc/offline
	for i in dmesg pstat showcfg; do \
		chgrp daemon $(DESTDIR)/etc/$$i; \
		chmod g+s $(DESTDIR)/etc/$$i; \
	done

mirror_install: $(MIRROR) 
	install -s $(MIRROR) $(MIRROR_DESTDIR)/etc
	chgrp daemon $(MIRROR_DESTDIR)/etc/mirror
	chmod g+s $(MIRROR_DESTDIR)/etc/mirror

clean:
	rm -f a.out core *.o
	for i in $(SUBDIR); do \
		(cd $$i; make $(MFLAGS) $(MRULES) clean); done
	rm -f $(STD) $(NSTD) $(KPRO) $(RPCSVC) $(MIRROR)

depend:
	cat </dev/null >x.c
	for i in $(STD) $(NSTD) $(RPCSVC) $(MIRROR); do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e '/\.\.\/h/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

arp		: arp.c
addbad		: addbad.c
bad144		: bad144.c
badsect		: badsect.c
bootflags	: bootflags.c
chown		: chown.c
clri		: clri.c
cron		: cron.c
diskpart	: diskpart.c
fsirand		: fsirand.c
gettable	: gettable.c
ifconfig	: ifconfig.c
init		: init.c
mknod		: mknod.c
mount		: mount.c
mrinval		: mrinval.c
on		: on.c
online		: online.c
dmesg		: dmesg.c
pmap		: pmap.c
portmap		: portmap.c
pstat		: pstat.c
renice		: renice.c
rmt		: rmt.c
route		: route.c
showcfg		: showcfg.c
umount		: umount.c
unmirror	: unmirror.c
update		: update.c
vipw		: vipw.c
vmtune		: vmtune.c

# Files listed in $(NSTD) have explicit make lines given below.
usclk_conf	: usclk_conf.c
	$(CC) -o usclk_conf  $(CFLAGS) usclk_conf.c -lseq

#
# we must link in an old version of sigvec so that these three programs
# can execute on 3.0.12 versions of i386 when doing deltas.  For ns32000
# just make it like normal.
#
halt		: $(MACHINE)halt
shutdown	: $(MACHINE)shutdown
reboot		: $(MACHINE)reboot

i386halt	: halt.c sigvec.o
	$(CC) -o halt $(CFLAGS) halt.c sigvec.o

i386reboot	: reboot.c sigvec.o
	$(CC) -o reboot $(CFLAGS) reboot.c sigvec.o

i386shutdown	: shutdown.c sigvec.o
	$(CC) -o shutdown $(CFLAGS) shutdown.c sigvec.o -lrpcsvc

ns32000halt	: halt.c
	$(CC) -o halt $(CFLAGS) halt.c
	
ns32000reboot	: reboot.c
	$(CC) -o reboot $(CFLAGS) reboot.c
	
ns32000shutdown	: shutdown.c
	$(CC) -o shutdown $(CFLAGS) shutdown.c -lrpcsvc
	
sigvec.o	: sigvec.s
	$(CPP) -E -UPROF $(INCLUDE) sigvec.s | $(AS) -o sigvec.o && \
	$(LD) -x -r sigvec.o -o sigvec.O && mv sigvec.O sigvec.o

mirror		: mirror.c 
	$(CC) -o mirror $(MIRROR_INCLUDE) $(CFLAGS) mirror.c -Z98304 -lpps

# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
