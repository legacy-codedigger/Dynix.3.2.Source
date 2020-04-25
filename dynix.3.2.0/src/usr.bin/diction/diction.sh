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

# $Header: diction.sh 2.1 90/07/24 $
D=/usr/bin
B=/usr/lib
echo $*
rest=
flag=
nflag=
mflag=-me
lflag=-ml
kflag=
file=
for i
do case $i in
 -f) flag=-f;shift; file=$1; shift; continue;;
-n) nflag=-n;shift; continue;;
-k) kflag=-k;shift; continue;;
 -mm) mflag=$1; shift; continue;;
-ms) mflag=$1;shift;continue;;
-me) mflag=$1;shift;continue;;
-ma) mflag=$1;shift;continue;;
-ml) lflag=$1;shift;continue;;
*) rest=$*; break;;
esac
done
 $D/deroff $kflag $lflag $mflag $rest|$B/dprog -d $nflag $flag $file
