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

# $Header: ypxfr_2perday.sh 1.2 87/05/28 $
#
# @(#)ypxfr_2perday.sh 1.1 86/02/05 Copyr 1985 Sun Microsystems, Inc.  
# @(#)ypxfr_2perday.sh	2.1 86/04/16 NFSSRC
#
# ypxfr_2perday.sh - Do twice-daily yp map check/updates
#

# set -xv
/etc/yp/ypxfr hosts.byname
/etc/yp/ypxfr hosts.byaddr
/etc/yp/ypxfr ethers.byaddr
/etc/yp/ypxfr ethers.byname
/etc/yp/ypxfr netgroup
/etc/yp/ypxfr netgroup.byuser
/etc/yp/ypxfr netgroup.byhost
