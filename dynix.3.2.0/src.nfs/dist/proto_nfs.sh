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

# $Header: proto_nfs.sh 1.5 89/09/25 $
#
# $Log:	proto_nfs.sh,v $

# Shell script to build a prototype filesystem for NFS build

ident=`whoami`
chown=/etc/chown
chgrp=/bin/chgrp
chmod=/bin/chmod

if [ "$ident" != "root" ]; then
	echo "proto_nfs.sh: You are not \"root\""
	echo "proto_nfs.sh: Correct owners and groups will be lost"
	chown=: chgrp=:
fi

# Loop through directories building them
# If run as root, restore the owner and group
# being careful to do ownership first as this may clear permission bits

while read mode owner group dir
do
	mkdir $dir
	( $chown $owner $dir && $chgrp $group $dir && $chmod $mode $dir ) &
done << '_P_'
755	root	daemon	tmpos_nfs
755	root	daemon	tmpos_nfs/etc
755	root	daemon	tmpos_nfs/usr
755	root	daemon	tmpos_nfs/usr/bin
755	root	daemon	tmpos_nfs/usr/etc
755	root	daemon	tmpos_nfs/usr/etc/yp
755	root	daemon	tmpos_nfs/usr/man
755	root	daemon	tmpos_nfs/usr/man/man1
755	root	daemon	tmpos_nfs/usr/man/man2
755	root	daemon	tmpos_nfs/usr/man/man3
755	root	daemon	tmpos_nfs/usr/man/man4
755	root	daemon	tmpos_nfs/usr/man/man5
755	root	daemon	tmpos_nfs/usr/man/man6
755	root	daemon	tmpos_nfs/usr/man/man7
755	root	daemon	tmpos_nfs/usr/man/man8
777	root	daemon	tmpos_nfs/usr/man/cat1
777	root	daemon	tmpos_nfs/usr/man/cat2
777	root	daemon	tmpos_nfs/usr/man/cat3
777	root	daemon	tmpos_nfs/usr/man/cat4
777	root	daemon	tmpos_nfs/usr/man/cat5
777	root	daemon	tmpos_nfs/usr/man/cat6
777	root	daemon	tmpos_nfs/usr/man/cat7
777	root	daemon	tmpos_nfs/usr/man/cat8
755	root	daemon	tmpos_nfs/usr/sys
_P_

(cd tmpos_nfs; ln -s usr/sys sys)

/bin/cat > tmpos_nfs/etc/exports << '_P_'
_P_
$chown     root tmpos_nfs/etc/exports &
$chgrp   daemon tmpos_nfs/etc/exports &
$chmod      644 tmpos_nfs/etc/exports &
wait
/bin/cat > tmpos_nfs/etc/NFS.OPTIONS << '_P_'
# Program Name		Option		Value
rpc.mountd		nfs_portmon	0
_P_
$chown     root tmpos_nfs/etc/NFS.OPTIONS &
$chgrp   daemon tmpos_nfs/etc/NFS.OPTIONS &
$chmod      644 tmpos_nfs/etc/NFS.OPTIONS &
wait
