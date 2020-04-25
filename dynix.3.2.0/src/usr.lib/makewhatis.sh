#! /bin/sh
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

# $Header: makewhatis.sh 2.1 90/03/21 $
rm -f /tmp/whatis$$
cd ${DESTDIR}/usr/man
for i in man1 man2 man3 man4 man5 man6 man7 man8
do
	cd $i
	/usr/lib/getNAME *.*
	cd ..
done >/tmp/whatis$$
ex - /tmp/whatis$$ <<\!
g/\\-/s//#/
g/\\\*-/s//#/
g/ VAX-11/s///
1,$s/.TH [^ ]* \([^ 	]*\).*	\([^#]*\)/\2(\1)	/
g/	 /s//	/g
g/#/s//-/
w
q
!
/usr/ucb/expand -24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100 /tmp/whatis$$ | sort | /usr/ucb/unexpand -a > ${DESTDIR}/usr/lib/whatis
chmod 644 ${DESTDIR}/usr/lib/whatis
rm -f /tmp/whatis$$
