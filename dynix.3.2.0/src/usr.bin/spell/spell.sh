#! /bin/sh
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

# $Header: spell.sh 2.1 1991/06/21 18:04:52 $
# V data for -v, B flags, D dictionary, S stop, H history, F files, T temp
V=/dev/null		B=			F= 
S=/usr/dict/hstop	H=/dev/null		T=/tmp/spell.$$
next="F=$F@"
trap "rm -f $T ${T}a ; exit" 0
for A in $*
do
	case $A in
	-v)	B="$B@-v"
		V=${T}a ;;
	-x)	B="$B@-x" ;;
	-b) 	D=${D-/usr/dict/hlistb}
		B="$B@-b" ;;
	-d)	next="D=" ;;
	-s)	next="S=" ;;
	-h)	next="H=" ;;
	-*)	echo "Bad flag for spell: $A"
		echo "Usage:  spell [ -v ] [ -b ] [ -d hlist ] [ -s hstop ] [ -h spellhist ]"
		exit ;;
	*)	eval $next"$A"
		next="F=$F@" ;;
	esac
done
IFS=@
case $H in
/dev/null)	deroff -w $F | sort -u | /usr/lib/spell $S $T |
		/usr/lib/spell ${D-/usr/dict/hlista} $V $B |
		sort -u +0f +0 - $T ;;
*)		deroff -w $F | sort -u | /usr/lib/spell $S $T |
		/usr/lib/spell ${D-/usr/dict/hlista} $V $B |
		sort -u +0f +0 - $T | tee -a $H
		who am i >> $H 2> /dev/null ;;
esac
case $V in
/dev/null)	exit ;;
esac
sed '/^\./d' $V | sort -u +1f +0
