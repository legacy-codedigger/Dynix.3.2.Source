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

# $Header: style.sh 2.0 86/01/28 $
L=/usr/lib
B=/usr/bin
echo " " $*
sflag=-s
eflag=
Pflag=
nflag=
lflag=
lcon=
rflag=
rcon=
mflag=-me
mlflag=-ml
kflag=
while :
do case $1 in
	-r) rflag=-r; shift; rcon=$1;shift;continue;;
	-l)lflag=-l; shift; lcon=$1;shift;continue;;
	-mm) mflag=-mm;shift;continue;;
	-ms) mflag=-ms;shift;continue;;
	-me) mflag=-me;shift;continue;;
	-ma) mflag=-ma;shift;continue;;
	-li|-ml) mlflag=-ml;shift;continue;;
	+li|-tt)mlflag=;shift;continue;;
	-p) sflag=-p;shift;continue;;
	-a) sflag=-a;shift;continue;;
	-e) eflag=-e;shift;continue;;
	-P) Pflag=-P;shift;continue;;
	-n) nflag=-n;shift;continue;;
	-N) nflag=-N;shift;continue;;
	-k) kflag=-k;shift;continue;;
	-flags) echo $0 "[-flags] [-r num] [-l num] [-e] [-p] [-n] [-N] [-a] [-P] [-mm|-ms] [-li|+li] [file ...]";exit;;
	-*) echo unknown style flag $i; exit;;
	*) break;;
esac
done
$B/deroff $kflag $mflag $mlflag $*^$L/style1^$L/style2^$L/style3 $rflag $rcon $lflag $lcon $sflag $nflag $eflag $Pflag
