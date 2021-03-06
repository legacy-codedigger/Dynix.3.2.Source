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
#ident $Header: maxtor.geom 1.2 90/03/29 $
#
# Disk geometry tables. 
# Key:
#	ns	#sectors/track
#	nt	#tracks/cylinder
#	nc	#cylinders/disk
#	rm	#revolutions per minute
#	cy	#sectors/cylinder
#	dc	#sectors/disk
#       xc      minimum capacity - only used on disks with a "mobile" capacity
#
maxtor1140|maxtor140|maxtor|Maxtor model XT1140 with interleave 1-1:\
	:ns#17:nt#15:nc#918:rm#3600:cy#255:dc#234090:xc#233324:
