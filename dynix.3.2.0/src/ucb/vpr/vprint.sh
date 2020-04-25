#! /bin/csh -f
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

# $Header: vprint.sh 2.0 86/01/28 $
set flags = ()
set printer = -Pvarian
top:
	if ($#argv > 0) then
		switch ($argv[1])
		case -V:
			set printer = -Pvarian
			shift argv
			goto top
		case -W:
			set printer = -Pversatec
			shift argv
			goto top
		case -*:
			set flags = ($flags $argv[1])
			shift argv
			goto top
		endsw
	endif
/usr/ucb/lpr $printer -p $flags $*
