
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

DLIB = $(DESTDIR)/usr/lib
UUCP = $(DLIB)/uucp
CRONTABS = $(DESTDIR)/usr/spool/cron/crontabs
LIBCRON = $(DLIB)/cron
INSDIR = $(DESTDIR)/etc

all::	.proto L-devices L-dialcodes L.cmds L.sys adm at.allow checkall \
	cron.allow filesave group issue motd profile queuedefs root \
	shutdown sys tapesave uucp uudemon.day uudemon.hr uudemon.wk \
	bcheckrc brc checklist gettydefs inittab passwd powerfail \
	rc USERFILE ioctl.syscon null

clean clobber:

install:
	make $(MFLAGS) $(MRULES) $(ARGS)

adm::
	install -c -m 644 adm $(CRONTABS)/adm

root::
	install -c -m 644 root $(CRONTABS)/root

sys::
	install -c -m 644 sys $(CRONTABS)/sys

uucp::
	install -c -m 644 uucp $(CRONTABS)/uucp

.proto::
	install -c -m 744 .proto $(LIBCRON)/.proto

at.allow::
	install -c -m 644 at.allow $(LIBCRON)/at.allow

cron.allow::
	install -c -m 644 cron.allow $(LIBCRON)/cron.allow

queuedefs::
	install -c -m 644 queuedefs $(LIBCRON)/queuedefs


bcheckrc::
	cd balance; \
	install -c -m 744 bcheckrc.sh $(INSDIR)/bcheckrc;

brc::
	cd balance;\
	install -c -m 744 brc.sh $(INSDIR)/brc;

checkall:	checkall.sh
	install -c -m 744 checkall.sh $(INSDIR)/checkall

checklist::
	cd balance;\
	install -c -m 664 checklist $(INSDIR)/checklist;

filesave:	filesave.sh
	install -c -m 744 filesave.sh $(INSDIR)/filesave

gettydefs::
	cd balance; \
	install -c -m 644 gettydefs.sh $(INSDIR)/gettydefs;

group::
	install -c -m 644 group $(INSDIR)/group

inittab::
	cd balance;\
	install -c -m 644 inittab.sh $(INSDIR)/inittab;

ioctl.syscon::
	cd balance; \
	install -c -m 644 ioctl.syscon $(INSDIR)/ioctl.syscon;

motd:
	cd balance;\
	install -c -m 644 motd.sh $(INSDIR)/motd

issue:
	install -c -m 644 issue.sh $(INSDIR)/issue

passwd::
	cd balance;\
	install -c -m 644 passwd $(INSDIR)/passwd;

powerfail::
	cd balance;\
	install -c -m 744 powerfail.sh $(INSDIR)/powerfail;

profile:	profile.sh
	install -c -m 644 profile.sh $(INSDIR)/profile

rc::
	cd balance;\
	install -c -m 744 rc.sh $(INSDIR)/rc;

shutdown:	shutdown.sh
	install -c -m 744 shutdown.sh $(INSDIR)/shutdown

tapesave:	tapesave.sh
	install -c -m 744 tapesave.sh $(INSDIR)/tapesave

uudemon.hr:	uudemon.hr.sh
	install -c -m 755 -o uucp uudemon.hr.sh $(UUCP)/uudemon.hr

uudemon.day:	uudemon.day.sh
	install -c -m 755 -o uucp uudemon.day.sh $(UUCP)/uudemon.day

uudemon.wk:	uudemon.wk.sh
	install -c -m 755 -o uucp uudemon.wk.sh $(UUCP)/uudemon.wk

L-devices::
	install -c -m 644 -o uucp L-devices $(UUCP)/L-devices

L-dialcodes::
	install -c -m 644 -o uucp L-dialcodes $(UUCP)/L-dialcodes

L.cmds::
	install -c -m 444 -o uucp L.cmds $(UUCP)/L.cmds

L.sys::
	install -c -m 400 -o uucp L.sys $(UUCP)/L.sys

USERFILE::
	install -c -m 400 -o uucp USERFILE $(UUCP)/USERFILE

null:
	install -c -m 644 /dev/null $(DESTDIR)/etc/log/filesave.log
	install -c -m 644 /dev/null $(DESTDIR)/etc/mnttab
	install -c -m 664 /dev/null $(DESTDIR)/etc/utmp
	install -c -m 664 /dev/null $(DESTDIR)/etc/wtmp
	install -c -m 664 /dev/null $(DESTDIR)/usr/adm/pacct
	install -c -m 600 /dev/null $(DESTDIR)/usr/adm/sulog
	install -c -m 664 /dev/null $(DESTDIR)/usr/lib/cron/log
	install -c -m 666 /dev/null $(DESTDIR)/usr/lib/spell/spellhist
	install -c -m 644 -o uucp /dev/null $(DESTDIR)/usr/lib/uucp/L_stat
	install -c -m 644 -o uucp /dev/null $(DESTDIR)/usr/lib/uucp/L_sub
	install -c -m 644 -o uucp /dev/null $(DESTDIR)/usr/lib/uucp/R_stat
	install -c -m 644 -o uucp /dev/null $(DESTDIR)/usr/lib/uucp/R_sub
