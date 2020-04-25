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

# $Header: mklost+found.sh 2.0 86/01/28 $
mkdir lost+found
cd lost+found
echo creating slots...
for i in 1 2 3 4 5 6 7 8 9 0 a b c d e f
do
	tee ${i}1 ${i}2 ${i}3 ${i}4 ${i}5 ${i}6 ${i}7 ${i}8 < /dev/null
	tee ${i}9 ${i}a ${i}b ${i}c ${i}d ${i}e ${i}f ${i}0 < /dev/null
done
echo removing dummy files...
for i in 1 2 3 4 5 6 7 8 9 0 a b c d e f
do
	rm ${i}1 ${i}2 ${i}3 ${i}4 ${i}5 ${i}6 ${i}7 ${i}8
	rm ${i}9 ${i}a ${i}b ${i}c ${i}d ${i}e ${i}f ${i}0
done
cd ..
echo done
ls -ld `pwd`/lost+found
