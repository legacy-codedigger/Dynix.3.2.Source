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

# $Header: Makefile 2.10 91/02/11 $
#
#	Makefile for sendmail base directory
#
# Note: newsyslog and sendmail.hf are now installed by proto.sh
DIRS=	${DESTDIR}/usr/spool/mqueue ${DESTDIR}/usr/lib/cf
ALLDIRS=include adm src aux md cf doc adb test
SENDMAIL=/usr/lib/sendmail
SYSLOG=	/usr/spool/mqueue/syslog
HOST=	myhost
# Note: CF_FILES must be updated if any files are added in the cf directory,
#       since the install action uses this list.
CF_FILES= KEY M4_KEY README

all:	src lib aux cf doc

aux:	FRC
	cd aux && make $(MFLAGS) $(MRULES)

lib:	FRC
#	cd $@ && m4 Makefile.m4 > Makefile && make $(MFLAGS) $(MRULES)

src:	FRC
	cd src && make $(MFLAGS) $(MRULES)

cf:	FRC
	cd cf/cf &&  make $(MFLAGS) $(MRULES)

doc:	FRC
#	cd doc && make $(MFLAGS) $(MRULES)

install:
	cd src && make $(MFLAGS) $(MRULES) install
	cd aux && make $(MFLAGS) $(MRULES) install
	cd cf && install -c -m 644 $(CF_FILES) $(DESTDIR)/usr/lib/cf
	cd cf/cf && install -c -m 644 * $(DESTDIR)/usr/lib/cf/cf
	cd cf/m4 && install -c -m 644 * $(DESTDIR)/usr/lib/cf/m4
	cd cf/sitedep && install -c -m 644 * $(DESTDIR)/usr/lib/cf/sitedep
	install -c -o daemon -m 644 /dev/null $(DESTDIR)$(SYSLOG)
	install -c -o daemon -m 644 /dev/null $(DESTDIR)$(SYSLOG).0
	install -c -o daemon -m 644 /dev/null $(DESTDIR)$(SYSLOG).1
	install -c -o daemon -m 644 /dev/null $(DESTDIR)$(SYSLOG).2
	install -c -o daemon -m 644 /dev/null $(DESTDIR)$(SYSLOG).3
	install -c -o daemon -m 644 /dev/null $(DESTDIR)$(SYSLOG).4
	install -c -o daemon -m 644 /dev/null $(DESTDIR)$(SYSLOG).5
	install -c -o daemon -m 644 /dev/null $(DESTDIR)$(SYSLOG).6
	install -c -o daemon -m 644 /dev/null $(DESTDIR)$(SYSLOG).7
	install -c -m 666	/dev/null	$(DESTDIR)$(SENDMAIL).st
	install -c -m 644	/dev/null	$(DESTDIR)$(SENDMAIL).fc
	install -c -m 644	cf/cf/uucpproto.cf	$(DESTDIR)$(SENDMAIL).cf
	rm -f					$(DESTDIR)/usr/ucb/newaliases
	ln -s			$(SENDMAIL)	$(DESTDIR)/usr/ucb/newaliases
	rm -f					$(DESTDIR)/usr/ucb/mailq
	ln -s			$(SENDMAIL)	$(DESTDIR)/usr/ucb/mailq
#	rm -f					$(DESTDIR)/usr/ucb/mailstats
#	ln -s			$(SENDMAIL)	$(DESTDIR)/usr/ucb/mailstats

# $(DESTDIR)/usr/spool/mqueue: FRC
# 	-mkdir $(DESTDIR)/usr/spool/mqueue
# 	chown root $(DESTDIR)/usr/spool/mqueue
# 	chgrp daemon $(DESTDIR)/usr/spool/mqueue
# 	chmod 750 $(DESTDIR)/usr/spool/mqueue

# $(DESTDIR)/usr/lib/cf: FRC
# 	-mkdir $(DESTDIR)/usr/lib/cf
# 	chown root $(DESTDIR)/usr/lib/cf
# 	chgrp daemon $(DESTDIR)/usr/lib/cf
# 	chmod 755 $(DESTDIR)/usr/lib/cf

clean:
#	cd lib && m4 Makefile.m4 > Makefile && make $(MFLAGS) $(MRULES) clean
	cd src && make $(MFLAGS) $(MRULES) clean
	cd aux && make $(MFLAGS) $(MRULES) clean
	cd cf/cf  && make $(MFLAGS) $(MRULES) clean
#	cd doc && make $(MFLAGS) $(MRULES) clean

FRC:
