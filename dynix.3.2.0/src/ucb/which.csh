#! /bin/csh
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

# $Header: which.csh 2.1 90/09/14 $
#
#	which : tells you which program you get
#
set noglob
foreach arg ( $argv )
    set alius = `alias $arg`
    switch ( $#alius )
	case 0 :
	    breaksw
	case 1 :
	    set arg = $alius[1]
	    breaksw
        default :
	    echo ${arg}: "	" aliased to $alius
	    continue
    endsw
    unset found
    if ( $arg:h != $arg:t ) then
	if ( -e $arg ) then
	    echo $arg
	else
	    echo $arg not found
	endif
	continue
    else
	foreach builtin ( "@" "alias" "alloc" "bg" "break" "breaksw" "case" "cd"\
	"chdir" "continue" "default" "dirs" "echo" "else" "end" "endif" "endsw"\
	"eval" "exec" "exit" "fg" "foreach" "glob" "goto" "hashstat" "history" \
	"if" "jobs" "kill" "limit" "login" "logout" "nice" "nohup" "notify" \
	"onintr" "popd" "pushd" "rehash" "repeat" "set" "setenv" "shift" \
	"source" "stop" "suspend" "switch" "time" "umask" "unalias" "unhash" \
	"unlimit" "unset" "unsetenv" "wait" "while")
		if ( $builtin =~ $arg ) then
			echo ${builtin}: 'csh builtin command (see csh man page)'
			set found
			break
		endif
	end

	if ( $?found ) then
		continue
	endif

	foreach i ( $path )
	    if ( -x $i/$arg && ! -d $i/$arg ) then
		echo $i/$arg
		set found
		break
	    endif
	end
    endif
    if ( ! $?found ) then
	echo no $arg in $path
    endif
end
