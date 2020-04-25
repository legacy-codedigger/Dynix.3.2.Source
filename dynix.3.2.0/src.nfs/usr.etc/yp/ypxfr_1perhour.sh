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

# $Header: ypxfr_1perhour.sh 1.1 86/12/18 $
#
# @(#)ypxfr_1perhour.sh 1.1 86/02/05 Copyr 1985 Sun Microsystems, Inc.  
# @(#)ypxfr_1perhour.sh	2.1 86/04/16 NFSSRC
#
# ypxfr_1perhour.sh - Do hourly yp map check/updates
#

# set -xv
/etc/yp/ypxfr passwd.byname
/etc/yp/ypxfr passwd.byuid 
