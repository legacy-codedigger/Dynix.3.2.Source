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

# $Header: suggest.sh 2.0 86/01/28 $
trap 'rm $$; exit' 1 2 3 15
D=/usr/lib/explain.d
while echo "phrase?";read x
do
cat >$$ <<dn
/$x.*	/s/\(.*\)	\(.*\)/use "\2" for "\1"/p
dn
case $x in
[a-z]*)
sed -n -f $$ $D; rm $$;;
*) rm $$;;
esac
done
