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

# $Header: man.sh 2.0 86/01/28 $
cmd= sec= fil= opt= i= all=
cmd=n sec=\?
cd /usr/man
for i
do
	case $i in

	[1-8])
		sec=$i ;;
	-n)
		cmd=n ;;
	-t)
		cmd=t ;;
	-k)
		cmd=k ;;
	-e | -et | -te)
		cmd=e ;;
	-ek | -ke)
		cmd=ek ;;
	-ne | -en)
		cmd=ne ;;

	-w)
		cmd=where ;;
	-*)
		opt="$opt $i" ;;

	*)
		fil=`echo man$sec/$i.*`
		case $fil in
		man7/eqnchar.7)
			all="$all /usr/pub/eqnchar $fil" ;;

		*\*)
			echo $i not found 1>&2 ;;
		*)
			all="$all $fil" ;;
		esac
	esac
done
case $all in
	"")
		exit ;;
esac
case $cmd in

n)
	nroff $opt -man $all ;;
ne)
	neqn $all | nroff $opt -man ;;
t)
	troff $opt -man $all ;;
k)
	troff -t $opt -man $all | tc ;;
e)
	eqn $all | troff $opt -man ;;
ek)
	eqn $all | troff -t $opt -man | tc ;;

where)
	echo $all ;;
esac
