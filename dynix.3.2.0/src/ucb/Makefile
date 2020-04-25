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

# $Header: Makefile 2.22 91/03/13 $
#
CFLAGS	= -O $(INCLUDE) $(DEBUG)
#

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	Mail ex sendmail lpr compact error fed gprof eyacc ftp indent \
	more msgs netstat sysline talk tftp tset vgrind vpr compress \
	lisp pascal fsplit telnet 42talk rdist

# Shell scripts that need only be installed and are never removed.
#
SHSCRIPT= help print
CSHSCRIPT= which

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	apply arcv biff checknr chfn chsh colcrt colrm ctags expand\
	finger fold from gcore grep groups head last\
	lastcomm leave lock man mkstr printenv prmail pti rcp\
	reset rlogin rsh rwho sccs script soelim strings tail\
	unexpand unifdef users vmstat w whereis whoami\
	what wc xstr yes

# Programs that must run setuid to root
#
SETUID=	chfn chsh rcp rlogin rsh
SETGID= w vmstat gcore

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	clear ruptime ul logger

# C programs that use the rpcsvc library
#
RPCSVC=		rup rusers quota

all:&	$(SUBDIR) dbx $(STD) $(NSTD) $(RPCSVC)

$(SUBDIR): FRC
	cd $@ && make $(MFLAGS) $(MRULES)

# dbx is not in the utilities source distribution
dbx: FRC
	sh -c  'if [ -d dbx/$(MACHINE) ]; then \
		    cd dbx/$(MACHINE) && make $(MFLAGS) $(MRULES); \
		else \
			exit 0; \
	        fi'

FRC:

$(STD):
	$(CC) $(CFLAGS) -o $@ $@.c

$(RPCSVC):
	$(CC) $(CFLAGS) -o $@ $@.c -lrpcsvc

install:
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} install); done
	sh -c  'if [ -d dbx/$(MACHINE) ]; then \
			cd dbx/$(MACHINE) && make $(MFLAGS) $(MRULES) install; \
		else \
			exit 0; \
		fi'
	for i in ${SHSCRIPT}; do \
		(install -c $$i.sh ${DESTDIR}/usr/ucb/$$i); done
	for i in ${CSHSCRIPT}; do \
		(install -c $$i.csh ${DESTDIR}/usr/ucb/$$i); done
	install -s ${STD} ${NSTD} ${RPCSVC} ${DESTDIR}/usr/ucb
	-for i in ${SETUID}; do (/etc/chown root ${DESTDIR}/usr/ucb/$$i; \
		chmod 4755 ${DESTDIR}/usr/ucb/$$i); done
	-for i in ${SETGID}; do (/bin/chgrp daemon ${DESTDIR}/usr/ucb/$$i; \
		chmod 2755 ${DESTDIR}/usr/ucb/$$i); done
	rm -f ${DESTDIR}/usr/ucb/uptime ${DESTDIR}/usr/ucb/f
	ln ${DESTDIR}/usr/ucb/w ${DESTDIR}/usr/ucb/uptime
	ln ${DESTDIR}/usr/ucb/finger ${DESTDIR}/usr/ucb/f
	rm -f ${DESTDIR}/usr/ucb/apropos ${DESTDIR}/usr/ucb/whatis
	ln ${DESTDIR}/usr/ucb/man ${DESTDIR}/usr/ucb/apropos
	ln ${DESTDIR}/usr/ucb/man ${DESTDIR}/usr/ucb/whatis
	rm -f ${DESTDIR}/usr/ucb/u
	ln ${DESTDIR}/usr/ucb/users ${DESTDIR}/usr/ucb/u

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} ${MRULES} clean); done
	rm -f ${STD} ${NSTD} ${RPCSVC}

depend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
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

# Files listed in ${NSTD} have explicit make lines given below.

clear:
	${CC} -o clear ${CFLAGS} clear.c -ltermcap

ruptime:
	${CC} -o ruptime ${CFLAGS} ruptime.c -ltermlib

ul:
	${CC} -o ul ${CFLAGS} ul.c -ltermcap

logger:
	${CC} -o logger ${CFLAGS} logger.c -lseq

# DO NOT DELETE THIS LINE -- make depend uses it

apply: apply.c
arcv: arcv.c
biff: biff.c
checknr: checknr.c
chfn: chfn.c
chsh: chsh.c
colcrt: colcrt.c
colrm: colrm.c
ctags: ctags.c
expand: expand.c
finger: finger.c
fold: fold.c
from: from.c
gcore: gcore.c
grep: grep.c
groups: groups.c
head: head.c
last: last.c
lastcomm: lastcomm.c
leave: leave.c
lock: lock.c
man: man.c
mkstr: mkstr.c
printenv: printenv.c
prmail: prmail.c
pti: pti.c
quota: quota.c
rcp: rcp.c
reset: reset.c
rlogin: rlogin.c
rsh: rsh.c
rup: rup.c
rusers: rusers.c
rwho: rwho.c
sccs: sccs.c
script: script.c
soelim: soelim.c
strings: strings.c
symorder: symorder.c
tail: tail.c
unexpand: unexpand.c
unifdef: unifdef.c
users: users.c
vmstat: vmstat.c
w: w.c
whatis: whatis.c
whereis: whereis.c
whoami: whoami.c
whois: whois.c
what: what.c
wc: wc.c
xstr: xstr.c
yes: yes.c
clear: clear.c
ruptime: ruptime.c
ul: ul.c
logger: logger.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
