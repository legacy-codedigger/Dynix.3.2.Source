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

# $Header: struct.sh 2.0 86/01/28 $
trap "rm -f /tmp/struct*$$" 0 1 2 3 13 15
files=no
for i
do
	case $i in
	-*)	;;
	 *)	files=yes
	esac
done

case $files in
yes)
	/usr/lib/struct/structure $* >/tmp/struct$$
	;;
no)
	cat >/tmp/structin$$
	/usr/lib/struct/structure /tmp/structin$$ $* >/tmp/struct$$
esac &&
	/usr/lib/struct/beautify</tmp/struct$$
