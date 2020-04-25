#! /bin/sh
#
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
#

# $Header: pre-install.sh 1.9 90/08/16 $
#
# $Log:	pre-install.sh,v $

set $*		# canonicalise the arguments
PASS=$1
shift

case $PASS in
base_build ) # called during mkdeltadist to evaluate arguments for below
	# read does not work for 4.2 /bin/sh so use eval
	#
	eval `awk -f ../tools/sum.awk $1 `
	RB_NEW=$rb;RI_NEW=$ri;UB_NEW=$ub;UI_NEW=$ui 
	eval `awk -f ../tools/sum.awk $2 `
	RB_OLD=$rb;RI_OLD=$ri;UB_OLD=$ub;UI_OLD=$ui 
	NEED_RBLOCKS=`expr "$RB_NEW" - "$RB_OLD"`
	NEED_RINODES=`expr "$RI_NEW" - "$RI_OLD"`
	NEED_UBLOCKS=`expr "$UB_NEW" - "$UB_OLD"`
	NEED_UINODES=`expr "$UI_NEW" - "$UI_OLD"`
	if [ $NEED_RBLOCKS -lt 0 ] ; then NEED_RBLOCKS=0; fi
	if [ $NEED_RINODES -lt 0 ] ; then NEED_RINODES=0; fi
	if [ $NEED_UBLOCKS -lt 0 ] ; then NEED_UBLOCKS=0; fi
	if [ $NEED_UINODES -lt 0 ] ; then NEED_UINODES=0; fi
	echo $NEED_RBLOCKS $NEED_RINODES $NEED_UBLOCKS $NEED_UINODES 
	;;
nfs_build ) # called during mknfsdist to evaluate arguments for below
	# read does not work for 4.2 /bin/sh so use eval
	#
	eval `awk -f ../tools/sum.awk $1 `
	RB_NEW=$rb;RI_NEW=$ri;UB_NEW=$ub;UI_NEW=$ui 
	NEED_RBLOCKS="$RB_NEW"
	NEED_RINODES="$RI_NEW"
	NEED_UBLOCKS="$UB_NEW"
	NEED_UINODES="$UI_NEW"
	if [ $NEED_RBLOCKS -lt 0 ] ; then NEED_RBLOCKS=0; fi
	if [ $NEED_RINODES -lt 0 ] ; then NEED_RINODES=0; fi
	if [ $NEED_UBLOCKS -lt 0 ] ; then NEED_UBLOCKS=0; fi
	if [ $NEED_UINODES -lt 0 ] ; then NEED_UINODES=0; fi
	echo $NEED_RBLOCKS $NEED_RINODES $NEED_UBLOCKS $NEED_UINODES 
	;;
pass1 ) # called during preview
	shift	# pre-install.sh
	/etc/mount -f /		# make sure root is in /etc/mtab for df
	product=$1
	NEED_RBLOCKS=$2 export NEED_RBLOCKS
	NEED_RINODES=$3	export NEED_RINODES
	NEED_UBLOCKS=$4	export NEED_UINODES
	NEED_UINODES=$5	export NEED_UINODES
	CONS=${6-/dev/console}
	(

	RABLOCKS=`/bin/df -i / | awk '{ if ( NR == 2 ) print $2 }'`
	RUBLOCKS=`/bin/df -i / | awk '{ if ( NR == 2 ) print $3 }'`
	RINODES=`/bin/df -i / | awk '{ if ( NR == 2 ) print $7 }'`
	RBLOCKS=`expr $RABLOCKS - $RUBLOCKS`
	UBLOCKS=`/bin/df -i /usr | awk '{ if ( NR == 2 ) print $4 }'`
	UINODES=`/bin/df -i /usr | awk '{ if ( NR == 2 ) print $7 }'`


	BAD=0
	if [ $RBLOCKS -lt $NEED_RBLOCKS ]
	then
		echo
		echo " The root file system has insufficient free blocks to install all the files." 
		echo " This delta requires $NEED_RBLOCKS blocks and only $RBLOCKS are available."
		BAD=1
	fi
	if [ $RINODES -lt $NEED_RINODES ]
	then
		echo
		echo " The root file system has insufficient free inodes to install all the files."
		echo " This delta requires $NEED_RINODES inodes and only $RINODES are available."
		BAD=1
	fi
	if [ $UBLOCKS -lt $NEED_UBLOCKS ]
	then
		echo
		echo " The /usr file system has insufficient free blocks to install all the files."
		echo " This delta requires $NEED_UBLOCKS blocks and only $UBLOCKS are available."
		BAD=1
	fi
	if [ $UINODES -lt $NEED_UINODES ]
	then
		echo
		echo " The /usr file system has insufficient free inodes to install all the files."
		echo " This delta requires $NEED_UINODES inodes and only $UINODES are available."
		BAD=1
	fi
	if [ "$BAD" = 1 ]
	then
		if [ -f bad_preinstall ]
		then
			echo " If you have already removed entries from list.delta"
			echo " Then you will need to skip this check to procede."
			echo " Otherwise you will need to remove some files or edit the list.delta file"
		else
			echo " You will need to remove some files or edit the list.delta file"
		fi
		echo " to increase disk capacity or reduce the requirements"
		echo
		touch bad_preinstall
	else
		rm -f bad_preinstall
	fi
	exit $BAD
	) >${CONS}
	;;
pass2 )
	# do nothing
	;;
backout )
	# do nothing
	;;
esac
