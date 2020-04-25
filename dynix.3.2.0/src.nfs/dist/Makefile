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

# $Header: Makefile 1.1 87/05/11 $
#
# Build prototype filesystem prior to a build of NFS
#

all		: proto_nfs
proto_nfs	: proto_nfs.sh
		  sh -c 'if [ ! -d ../../$(MACHINE) ]; then \
			umask 022 && mkdir ../../$(MACHINE); \
		  fi; exit 0'
		  cd ../.. && rm -rf \
			$(MACHINE)/tmpos_nfs ; \
		  cd $(MACHINE) && \
	  	    sh ../src.nfs/dist/proto_nfs.sh
clean install	:
