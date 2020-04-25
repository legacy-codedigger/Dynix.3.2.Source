#!/bin/sh
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

#			Summary
#
# The ATS FORTRAN run-time libraries (libf) are distributed so they may be
# used on all versions of DYNIX.  As distributed, the libraries are configured
# for the versions of DYNIX before DYNIX V3.2.  When installed on DYNIX V3.2
# and later DYNIX versions, the libraries must be configured by configlibf,
# this script.
#
#			Details
#
# The ucb-universe 387 libf libraries have a file, values.o, which has pre-V3.2
# libm constants in it.  The DYNIX V3.2 387 libm values.o defines these
# constants as well as others, so the definitions in libf are not needed on
# DYNIX V3.2, and worse, are incompatible with the DYNIX V3.2 libm.
#
# The ucb-universe libf libraries have another file, ucb_f100.o, which has
# pre-V3.2 stdio routines modified to enable up to 100 files being open
# simultaneously.  The DYNIX V3.2 stdio supports 100 open files with no
# modifications, so the routines in ucb_f100.o are not needed on DYNIX V3.2,
# and worse, are incompatible with the DYNIX V3.2 stdio.
#
# The att universe libraries have no such incompatibility and require no
# installation-time modifications.
#
# Configlibf alters the ucb libf libraries by deleting values.o and ucb_f100.o,
# replacing them with v32_ucb_f100.o, the V3.2-compatible replacement for
# ucb_f100.o.  This action is selected by the "V3.2" configlibf option.
# When removing values.o and ucb_f100.o from each library, configlibf leaves
# them in the directory so the library can be restored to its original state
# if needed.
#
# Configlibf can undo the above actions.  It locates the saved values.o and
# ucb_f100.o files and replaces them in the appropriate libraries.  This action
# is selected by the "V3.1" configlibf option.
#
#			Usage
#
#	configlibf -v [ V3.1 | V3.2 | native ] [ all | fortran_version ... ]
#
# Optional:
# -v		Verbose execution.  Report actions as they happen.
#
# Required: [ V3.1 | V3.2 | native ]
# V3.1		Restore ATS FORTRAN library for use on DYNIX V3.1
#		or earlier DYNIX versions (pre-V3.2, really.)
# V3.2		Convert ATS FORTRAN library for use on DYNIX V3.2
#		or later DYNIX versions.
# native	Configure ATS FORTRAN library for use on current system
#		as determined by /etc/version.
#
# Required: [ all | fortran_version ... ]
# all			Configure all libraries in /usr/xltr/fortran
# fortran_version	Configure libraries in /usr/xltr/fortran/<fortran_version>

# @(#)$Header: configlibf 1.1 1991/10/04 20:44:21 $

# set execution environment
set -u		# undefined sh variables are an error
umask 133	# files created with -rw-r--r--
PATH=/bin:/usr/bin:/etc; export PATH

# set program name and usage strings
prog=`basename $0`
usage1="Usage: $prog -v [ V3.1 | V3.2 | native ] [ all | fortran_version ... ]"
usage2="Read the $0 script for option details"

# set some other strings
V31="$prog: V3.1 - Restore ATS FORTRAN library for use on DYNIX V3.1 or earlier"
V32="$prog: V3.2 - Convert ATS FORTRAN library for use on DYNIX V3.2 or later"
v31_obj1=values.o
v31_obj2=ucb_f100.o
v32_obj2=v3.2_$v31_obj2
v32_src2=`basename $v32_obj2 .o`.c
probe=$prog.probe.$$

# verify we're on a dynix3 system
if [ ! -f /dynix ]
then
	echo $prog: 'System must be DYNIX 3.*'
	exit 1
fi

# check optional arg
verbose=no
if [ $# -gt 0 ]
then 
	if [ "$1" = -v ]
	then
		verbose=yes
		shift
	fi
fi

# must have at least two args remaining
if [ $# -lt 2 ]
then
	echo $usage1
	echo $usage2
	exit 1
fi

# get goal
goal=$1
case $goal
in
	V3.2)
		;;
	V3.1)
		;;
	native)
		# determine goal
		goal=`version | awk '{ print $2 }'`
		case $goal
		in
			V3.[2-9])
				goal=V3.2
				if [ $verbose = yes ]
				then
					echo $V32
				fi
				;;
			V3.[2-9].*)
				goal=V3.2
				if [ $verbose = yes ]
				then
					echo $V32
				fi
				;;
			V3.[0-1])
				goal=V3.1
				if [ $verbose = yes ]
				then
					echo $V31
				fi
				;;
			V3.[0-1].*)
				goal=V3.1
				if [ $verbose = yes ]
				then
					echo $V31
				fi
				;;
			*)
				echo $prog: Unknown version: $goal
				echo $usage1
				exit 1
				;;
		esac
		;;
	*)
		echo $prog: Invalid argument: $goal
		echo $usage1
		echo $usage2
		exit 1
		;;
esac
shift

# get directories
if [ $# = 1 -a $1 = all ]
then
	# find fortran library directories
	places='/usr/xltr/fortran/*/lib/ucb'
	dirs=`echo $places`
	if [ "$dirs" = "$places" ]
	then
		echo $prog: Cannot find FORTRAN libraries. Looked for $places
		exit 1
	fi
else
	dirs=
	for dir in $*
	do
		full_dir=/usr/xltr/fortran/$dir/lib/ucb
		if [ -d $full_dir ]
		then
			dirs="$dirs $full_dir"
		else
			echo $prog: Not directory: $full_dir
		fi
	done # end for
fi

# set trap for probe file
trap "rm -f $probe; exit 1" 1 2 3 4 5 6 7 8

# make sure we have permission to write in the dirs
todo=
for dir in $dirs
do
	cd $dir
	if [ $? = 0 ]
	then
		rm -f $probe
		touch $probe 2> /dev/null
		if [ $? = 0 ]
		then
			todo="$todo $dir"
			rm -f $probe
		else
			echo $prog: Cannot write in $dir
		fi
	else
		echo $prog: Cannot chdir to $dir
	fi
done # for dir
dirs="$todo"
if [ -z "$dirs" ]
then
	exit 1
fi

# set trap for temps
trap "rm -f $v31_obj1 $v31_obj2 $v32_obj2 $v32_src2; exit 1" 1 2 3 4 5 6 7 8

# configure the libraries
# (we execute ar with the 'l' option since ranlib does that regardless)
for dir in $dirs
do
	cd $dir
	if [ $verbose = yes ]
	then
		echo $prog: Checking $dir ...
	fi
	todo=
	libs="libf.a libf_p.a libfpaf.a libfpaf_p.a"
	for lib in $libs
	do
		if [ ! -f $lib ]
		then
			echo $prog: Cannot read $dir/$lib
		else
			if [ ! -w $lib ]
			then
				echo $prog: Cannot write $dir/$lib
			else
				todo="$todo $lib"
			fi
		fi
	done # for lib
	libs="$todo"
	for lib in $libs
	do
		if [ $verbose = yes ]
		then
			echo $prog: Working on $dir/$lib ...
		fi
		there=no
		todo=
		case $goal
		in
			V3.1)
				#
				# values.o - 387 libraries only
				#
				if [ $lib = libf.a -o $lib = libf_p.a ]
				then
					# see if v3.1 file is in $lib
					ar t $lib $v31_obj1 > /dev/null 2>&1
					if [ $? = 0 ]
					then
						if [ $verbose = yes ]
						then
							echo $prog: $lib already contains $v31_obj1
						fi
						there=yes
					else
						# not in $lib - should already exist
						if [ -f $lib.$v31_obj1 ]
						then
							if [ $verbose = yes ]
							then
								echo $prog: Restoring $v31_obj1 to $lib
							fi
							rm -f $v31_obj1
							cp $lib.$v31_obj1 $v31_obj1
							ar rl $lib $v31_obj1
							rm -f $v31_obj1
							todo=ranlib
							there=yes
						else
							echo $prog: Cannot locate $v31_obj1.  $lib not configured.
						fi
					fi
				fi
				#
				# ucb_f100.o
				#
				# see if v3.1 file is in $lib
				ar t $lib $v31_obj2 > /dev/null 2>&1
				if [ $? = 0 ]
				then
					if [ $verbose = yes ]
					then
						echo $prog: $lib already contains $v31_obj2
					fi
					there=yes
				else
					# not in $lib - should already exist
					if [ -f $lib.$v31_obj2 ]
					then
						if [ $verbose = yes ]
						then
							echo $prog: Restoring $v31_obj2 to $lib
						fi
						rm -f $v31_obj2
						cp $lib.$v31_obj2 $v31_obj2
						ar rl $lib $v31_obj2
						rm -f $v31_obj2
						todo=ranlib
						there=yes
					else
						echo $prog: Cannot locate $v31_obj2.  $lib not configured.
					fi
				fi
				if [ $there = yes ]
				then
					# see if v3.2 file is in $lib too
					ar t $lib $v32_obj2 > /dev/null 2>&1
					if [ $? = 0 ]
					then
						# in $lib - delete it
						if [ $verbose = yes ]
						then
							echo $prog: Deleting $v32_obj2 from $lib
						fi
						ar dl $lib $v32_obj2
						todo=ranlib
					fi
				fi
				;;
			V3.2)
				#
				# values.o - 387 libraries only
				#
				if [ $lib = libf.a -o $lib = libf_p.a ]
				then
					# see if v3.1 file is in $lib
					ar t $lib $v31_obj1 > /dev/null 2>&1
					if [ $? = 0 ]
					then
						# in $lib - needs to be saved and deleted
						if [ ! -f $lib.$v31_obj1 ]
						then
							# save it
							if [ $verbose = yes ]
							then
								echo $prog: Saving $v31_obj1 from $lib as $lib.$v31_obj1
							fi
							ar x $lib $v31_obj1
							mv $v31_obj1 $lib.$v31_obj1
						fi
						# delete it from the library
						if [ $verbose = yes ]
						then
							echo $prog: Deleting $v31_obj1 from $lib
						fi
						ar dl $lib $v31_obj1
						todo=ranlib
					fi
				fi
				#
				# ucb_f100.o
				#
				# see if v3.2 file is in $lib
				ar t $lib $v32_obj2 > /dev/null 2>&1
				if [ $? = 0 ]
				then
					if [ $verbose = yes ]
					then
						echo $prog: $lib already contains $v32_obj2
					fi
					there=yes
				else
					# not here - does it already exist?
					if [ ! -f $v32_obj2 ]
					then
						# Create the v3.2 replacement
						rm -f $v32_src2
						echo '#include <stdio.h>'	>  $v32_src2
						echo 'int _Nfiles_ = _NFILE;'	>> $v32_src2
						cc -c $v32_src2
						rm -f $v32_src2
						chgrp -f daemon $v32_obj2
						chown -f root $v32_obj2
					fi
					if [ $verbose = yes ]
					then
						echo $prog: Adding $v32_obj2 to $lib
					fi
					ar rl $lib $v32_obj2
					todo=ranlib
					there=yes
				fi
				if [ $there = yes ]
				then
					# see if v3.1 file is in $lib too
					ar t $lib $v31_obj2 > /dev/null 2>&1
					if [ $? = 0 ]
					then
						# in $lib - needs to be saved and deleted
						if [ ! -f $lib.$v31_obj2 ]
						then
							# save it
							if [ $verbose = yes ]
							then
								echo $prog: Saving $v31_obj2 from $lib as $lib.$v31_obj2
							fi
							ar x $lib $v31_obj2
							mv $v31_obj2 $lib.$v31_obj2
						fi
						# delete it from the library
						if [ $verbose = yes ]
						then
							echo $prog: Deleting $v31_obj2 from $lib
						fi
						ar dl $lib $v31_obj2
						todo=ranlib
					fi
				fi
				;;
		esac
		# ranlib if needed
		if [ "$todo" = ranlib ]
		then
			ranlib $lib
			chgrp -f daemon $lib
			chown -f root $lib
		fi
	done # for lib
	rm -f $v31_obj1 $v31_obj2 $v32_obj2 $v32_src2
done # for dir

exit 0
