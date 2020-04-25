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

# $Header: /usr/src/dynix.3.2.0/src/usr.bin/calendar/RCS/calendar.sh,v 1.2 92/12/11 02:00:13 bruce Exp $

PATH=/bin:/usr/bin:
tmp=/tmp/cal$$
pwdtmp=/tmp/calpwd$$
pwdtmp2=/tmp/cal2pwd$$
trap "rm -f $tmp $pwdtmp $pwdtmp2 /tmp/cal1$$ /tmp/cal2$$"
trap exit 1 2 13 15
/usr/lib/calendar >$tmp
case $# in
0)
	trap "rm -f $tmp ; exit" 0 1 2 13 15
	(/lib/cpp calendar | egrep -f $tmp);;
*)
	trap "rm -f $tmp $pwdtmp $pwdtmp2 /tmp/cal1$$ /tmp/cal2$$; exit" 0 1 2 13 15
	/bin/cp /etc/passwd $pwdtmp
	if [ -f /usr/bin/ypcat ]; then
		/usr/bin/ypcat passwd >> $pwdtmp 2> /dev/null
	fi
	/usr/bin/sort -u $pwdtmp > $pwdtmp2
	/bin/echo -n "Subject: Calendar for " > /tmp/cal1$$
	date | sed -e "s/ [0-9]*:.*//" >> /tmp/cal1$$
	sed '
		s/\([^:]*\):.*:\(.*\):[^:]*$/y=\2 z=\1/
	' $pwdtmp2 \
	| while read x
	do
		eval $x
		if test -r $y/calendar
		then
			(/lib/cpp $y/calendar | egrep -f $tmp) 2>/dev/null  > /tmp/cal2$$
			if test -s /tmp/cal2$$
			then
				cat /tmp/cal1$$ /tmp/cal2$$ | mail $z
			fi
		fi
	done
esac
