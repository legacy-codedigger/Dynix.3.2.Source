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

# $Header: plot.sh 2.0 86/01/28 $
PATH=/bin:/usr/bin
case $1 in
-T*)	t=$1
	shift ;;
*)	t=-T$TERM
esac
case $t in
-T450)	exec t450 $*;;
-T300)	exec t300 $*;;
-T300S|-T300s)	exec t300s $*;;
-Tver)	exec vplot $*;;
-Ttek|-T4014|-T)	exec tek $* ;;
*)  echo plot: terminal type $t not known 1>&2; exit 1
esac
