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

# $Header: vgrind.sh 2.0 86/01/28 $
#
# vgrind
#
set b=/usr/lib
set voptions=
set options=
set files=
set f=''
set head=""
top:
if ($#argv > 0) then
    switch ($1:q)

    case -f:
	set f='filter'
	set options = "$options $1:q"
	shift
	goto top

    case -t:
	set voptions = "$voptions -t"
	shift
	goto top

    case -o*:
	set voptions="$voptions $1:q"
	shift
	goto top

    case -W:
	set voptions = "$voptions -W"
	shift
	goto top

    case -d:
	if ($#argv < 2) then
	    echo "vgrind: $1:q option must have argument"
	    goto done
	else
	    set options = ($options $1:q $2)
	    shift
	    shift
	    goto top
	endif
			
    case -h:
	if ($#argv < 2) then
	    echo "vgrind: $1:q option must have argument"
	    goto done
	else
	    set head="$2"
	    shift
	    shift
	    goto top
	endif
			
    case -*:
	set options = "$options $1:q"
	shift
	goto top

    default:
	set files = "$files $1:q"
	shift
	goto top
    endsw
endif
if (-r index) then
    echo > nindex
    foreach i ($files)
	#	make up a sed delete command for filenames
	#	being careful about slashes.
	echo "? $i ?d" | sed -e "s:/:\\/:g" -e "s:?:/:g" >> nindex
    end
    sed -f nindex index >xindex
    if ($f == 'filter') then
	if ("$head" != "") then
	    $b/vfontedpr $options -h "$head" $files | cat $b/tmac/tmac.vgrind -
	else
	    $b/vfontedpr $options $files | cat $b/tmac/tmac.vgrind -
	endif
    else
	if ("$head" != "") then
	    $b/vfontedpr $options -h "$head" $files | \
		/bin/sh -c "vtroff -rx1 $voptions -i -mvgrind 2>> xindex"
	else
	    $b/vfontedpr $options $files | \
		/bin/sh -c "vtroff -rx1 $voptions -i -mvgrind 2>> xindex"
	endif
    endif
    sort -df +0 -2 xindex >index
    rm nindex xindex
else
    if ($f == 'filter') then
	if ("$head" != "") then
	    $b/vfontedpr $options -h "$head" $files | cat $b/tmac/tmac.vgrind -
	else
	    $b/vfontedpr $options $files | cat $b/tmac/tmac.vgrind -
	endif
    else
	if ("$head" != "") then
	    $b/vfontedpr $options -h "$head" $files \
		| vtroff -i $voptions -mvgrind
	else
	    $b/vfontedpr $options $files \
		| vtroff -i $voptions -mvgrind
	endif
    endif
endif

done: