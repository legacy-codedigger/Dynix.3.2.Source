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

# $Header: Makefile 2.20 1991/05/06 15:20:08 $
#
CFLAGS=	-O $(INCLUDE) $(DEBUG)
#

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	troff calendar dc diction diff3 eqn graph \
	host install.d learn lex m4 \
	plot refer spell struct tbl tip uucp yacc \
	prof whois
MDEP =  ddt
NEVER=	f77 efl ratfor lint

# Shell scripts that need only be installed and are never removed.
# NB: dropped from 4.2 BSD: man
SCRIPT=	nohup mailbug
SNOTYET=lorder

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	at basename cal cb checkeq col comm deroff \
	fgrep file find fpr iostat join look mesg ptx \
	rev sleep sort spline split sum tabs tc tk touch tr tsort tty \
	uniq units
CNOMORE=ar11

# C programs that live in the current directory and need explicit make lines.
# (make depend has special rules for these files)
#
NSTD=	bc egrep

all:&	${SUBDIR} ${MDEP} xsend ${STD} nstd crypt

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} ${MRULES}

${MDEP}	: FRC
	sh -c 'if [ "$(MACHINE)" = "ns32000" ]; then \
		cd $@; make $(MFLAGS) $(MRULES) ; else \
		echo "usr.bin: $@ not built for $(MACHINE)"; \
	fi'

xsend: FRC
	sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		cd xsend; make ${MFLAGS} ${MRULES}; \
	fi'

nstd:	$(NSTD)	# extra rule so that these guys aren't done in parallel

bc egrep:
	${YACC} ${YFLAGS} $@.y
	${CC} -c ${CFLAGS} y.tab.c
	mv y.tab.o $@.o
	${CC} -o $@ $@.o
	rm -f $@.o y.tab.c

# special rule here to get version of libc with real crypt entry points
crypt:
	sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		${CRYPT_CC} ${CFLAGS} -o crypt crypt.c; \
	fi'

crypt.c:
	
${STD}:
	${CC} ${CFLAGS} -o $@ $@.c

install: FRC
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} install); \
	done
	sh -c 'if [ "$(MACHINE)" = "ns32000" ]; then \
		for i in ${MDEP}; do \
			(cd $$i; make ${MFLAGS} ${MRULES} install); \
		done \
	fi; exit 0'
	sh -c 'if [ "${EXPORT}" = TRUE ]; then :; else \
		cd xsend; make ${MFLAGS} ${MRULES} install; \
	fi'
	for i in ${SCRIPT}; do \
		(install -c $$i.sh ${DESTDIR}/usr/bin/$$i); done
	install -s ${STD} ${NSTD} ${DESTDIR}/usr/bin
	sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		install -s crypt ${CRYPT_DESTDIR}/usr/bin/crypt; \
	fi'
	install -c -m 644 units.lib ${DESTDIR}/usr/lib/units
	chown root ${DESTDIR}/usr/bin/at
	chmod 4755 ${DESTDIR}/usr/bin/at
	chgrp daemon ${DESTDIR}/usr/bin/iostat
	chmod 2755 ${DESTDIR}/usr/bin/iostat

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} clean); \
	done
	sh -c 'if [ ! -d xsend ]; then :; else \
		(cd xsend; make ${MFLAGS} ${MRULES} clean); \
	fi'
	rm -f ${STD} ${NSTD}

FRC:

# beware of the non-standard stuff for bc and egrep
depend:
	cat </dev/null >x.c
	for i in ${STD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e '/\.\.\/h/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	for i in bc egrep; do \
		(echo $$i: $$i.y >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.y | sed \
			-e '/\.\.\/h/d' \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.y//' >>makedep); done
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

# DO NOT DELETE THIS LINE -- make depend uses it

# ar11: ar11.c
at: at.c
basename: basename.c
cal: cal.c
cb: cb.c
checkeq: checkeq.c
col: col.c
comm: comm.c
crypt: crypt.c
deroff: deroff.c
fgrep: fgrep.c
file: file.c
find: find.c
fpr: fpr.c
iostat: iostat.c
join: join.c
look: look.c
mesg: mesg.c
ptx: ptx.c
rev: rev.c
sleep: sleep.c
sort: sort.c
spline: spline.c
split: split.c
sum: sum.c
tabs: tabs.c
tc: tc.c
tk: tk.c
touch: touch.c
tr: tr.c
tsort: tsort.c
tty: tty.c
uniq: uniq.c
units: units.c
bc: bc.y
egrep: egrep.y
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
