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

# $Header: newvers.sh 2.6 1991/06/14 20:09:51 $
#
# newvers.sh
#	Generates version string for information when system boots.
#

# $Log: newvers.sh,v $

VERSION=3.2.0.i

if [ ! -r version ]; then echo 0 > version; fi
touch version
DATE=`date`
echo "#ifdef NFS" > vers.c
awk "	{	version = \$1 + 1; }\
END	{	printf \"char version[] = \\\"DYNIX(R) V3.2.0 NFS  #%d \", \
			version >> \"vers.c\";\
		printf \"%d\n\", version > \"version\"; }" < version
echo "($USER): $DATE"'\n";' >> vers.c
echo "#else !NFS" >> vers.c
awk "	{	version = \$1; }\
END	{	printf \"char version[] = \\\"DYNIX(R) V3.2.0  #%d \", \
			version >> \"vers.c\" }" < version
echo "($USER): $DATE"'\n";' >> vers.c
echo "#endif" >> vers.c
