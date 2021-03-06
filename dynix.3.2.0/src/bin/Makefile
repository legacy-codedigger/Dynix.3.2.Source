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

# $Header: /usr/src/dynix.3.2.0/src/bin/RCS/Makefile,v 1.2 1993/01/12 08:55:17 bruce Exp $
#
CFLAGS=	-O $(INCLUDE) $(DEBUG)
LDFLAGS= -lseq
#

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	awk csh diff make sed sh tp login

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	false true pascal fortran

# C programs that live in the current directory and do not need
# explicit make lines.
#
NFS=	domainname
STD=	cat chgrp chmod cmp date dd df du echo ed expr \
	grep hostid hostname id kill ln ls mail mkdir mt nice \
	od pagesize passwd pr pwd rm rmail rmdir stty su sync \
	tar tee test time wall who write universe $(NFS)

# Programs that must run setuid to root
#
SETUID=	chgrp mail passwd su

# Programs that must run setgid to ${DISK} (to read /dev/<disks>)
#
SETGID_DISK=	df
DISK=	Disk

# Programs that must run setgid to ${KMEM} (to read /dev/*mem*)
#
SETGID_KMEM=	ps
KMEM=	kmem

# C programs that live in the current directory and need explicit make lines.
#
NSTD= ps

all:&	${SUBDIR} ${STD} ${NSTD} cp mv ed.crypt

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} ${MRULES}

FRC:

${STD} mv cp:
	${CC} ${CFLAGS} -o $@ $@.c ${LDFLAGS}

# ed with "-x" encryption mode supported
ed.crypt: ed.c
	-if [ "$(EXPORT)" != TRUE ]; then \
		${CRYPT_CC} ${CFLAGS} -DCRYPT ed.c -o ed.crypt; \
	fi

# take care with cp and mv, install uses them
install:
	install -s cp ${DESTDIR}/bin/newcp; \
		mv ${DESTDIR}/bin/newcp ${DESTDIR}/bin/cp;
	rm -f ${DESTDIR}/bin/mv; cp mv ${DESTDIR}/bin/mv; \
		chmod 755 ${DESTDIR}/bin/mv;  rm -f mv; \
		${STRIP} ${DESTDIR}/bin/mv
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} install); done
	install -s ${STD} ${NSTD} ${DESTDIR}/bin
	cd $(DESTDIR)/bin && rm -f ptime && ln time ptime
	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/bin/$$i); done
	rm -f ${DESTDIR}/bin/att; ln ${DESTDIR}/bin/universe ${DESTDIR}/bin/att
	rm -f ${DESTDIR}/bin/ucb; ln ${DESTDIR}/bin/universe ${DESTDIR}/bin/ucb
	-for i in ${SETUID}; do (/etc/chown root ${DESTDIR}/bin/$$i; \
		chmod 4755 ${DESTDIR}/bin/$$i); done
	-for i in ${SETGID_DISK}; do \
		chgrp ${DISK} ${DESTDIR}/bin/$$i; \
		chmod g+s ${DESTDIR}/bin/$$i; done
	-for i in ${SETGID_KMEM}; do \
		chgrp ${KMEM} ${DESTDIR}/bin/$$i; \
		chmod g+s ${DESTDIR}/bin/$$i; done
	rm -f ${DESTDIR}/bin/[; ln ${DESTDIR}/bin/test ${DESTDIR}/bin/[
	rm -f ${DESTDIR}/bin/e; ln ${DESTDIR}/bin/ed ${DESTDIR}/bin/e
	-if [ "$(EXPORT)" != TRUE ]; then \
		install -s ed.crypt ${CRYPT_DESTDIR}/bin/ed; \
		rm -f {CRYPT_DESTDIR}/bin/e; \
		ln ${CRYPT_DESTDIR}/bin/ed ${CRYPT_DESTDIR}/bin/e; \
	fi
	rm -f ${DESTDIR}/bin/vax; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/vax
	rm -f ${DESTDIR}/bin/pdp11; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/pdp11
	rm -f ${DESTDIR}/bin/68k; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/68k
	rm -f ${DESTDIR}/bin/u3b; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/u3b
	rm -f ${DESTDIR}/bin/u3b10; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/u3b10
	rm -f ${DESTDIR}/bin/u3b2; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/u3b2
	rm -f ${DESTDIR}/bin/u3b5; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/u3b5
	rm -f ${DESTDIR}/bin/u370; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/u370
	rm -f ${DESTDIR}/bin/i386; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/i386
	rm -f ${DESTDIR}/bin/ns32000; \
		 ln ${DESTDIR}/bin/false ${DESTDIR}/bin/ns32000
	rm -f ${DESTDIR}/bin/$(MACHINE); \
		 ln ${DESTDIR}/bin/true ${DESTDIR}/bin/$(MACHINE)

clean:
	rm -f a.out core *.s *.o errs
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} ${MRULES} clean); done
	rm -f ${STD} ${NSTD} expr.c

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

# Files listed in $(NSTD) have explicit make lines given below.

ps		:; $(CC) $(CFLAGS) -o ps ps.c -lm


# DO NOT DELETE THIS LINE -- make depend uses it

ar: ar.c
cat: cat.c
cc: cc.c
chgrp: chgrp.c
chmod: chmod.c
cmp: cmp.c
date: date.c
dd: dd.c
df: df.c
du: du.c
echo: echo.c
ed: ed.c
expr: expr.c
grep: grep.c
hostid: hostid.c
hostname: hostname.c
id: id.c
kill: kill.c
ld: ld.c
ln: ln.c
ls: ls.c
mail: mail.c
mkdir: mkdir.c
mt: mt.c
nice: nice.c
nm: nm.c
od: od.c
pagesize: pagesize.c
passwd: passwd.c
pr: pr.c
pwd: pwd.c
rm: rm.c
rmail: rmail.c
rmdir: rmdir.c
size: size.c
strip: strip.c
stty: stty.c
su: su.c
sync: sync.c
tar: tar.c
tee: tee.c
test: test.c
time: time.c
wall: wall.c
who: who.c
write: write.c
ps: ps.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
