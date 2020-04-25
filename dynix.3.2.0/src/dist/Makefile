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

# $Header: Makefile 2.10 1991/07/02 22:16:08 $
#
# Build prototype filesystem prior to an entire build
#

all		: proto
proto		: proto.sh
		  sh -c 'if [ ! -d ../../$(MACHINE) ]; then \
			umask 022 && mkdir ../../$(MACHINE); \
		  fi; exit 0'
		  sh -c 'if [ "$(TMPOS_USR)" != ""  -a \
			      "$(TMPOS_ROOT)" != "" ]; then \
			/etc/umount /dev/$(TMPOS_USR); \
			/etc/umount /dev/$(TMPOS_ROOT); \
		  fi; exit 0'
		  cd ../.. && rm -rf \
			$(MACHINE)/tmpos 		\
			$(MACHINE)/tmpos_att 		\
			$(MACHINE)/tmpos_att_crypt 	\
			$(MACHINE)/tmpos_crypt 		\
			$(MACHINE)/tmpos_stripe		\
			$(MACHINE)/tmpos_mirror		\
			$(MACHINE)/tmpos_mfg  &&	\
		  PWD=`/bin/pwd` && \
		  sh -c 'if [ "$(TMPOS_USR)" != "" -a \
			      "$(TMPOS_ROOT)" != "" ]; then \
			/etc/newfs -s 30000 -i 8192 /dev/r$(TMPOS_ROOT) \
				"$(ROOT_DISKTYPE)"; \
			/etc/fsck /dev/r$(TMPOS_ROOT); \
			mkdir $(MACHINE)/tmpos; \
			/etc/mount /dev/$(TMPOS_ROOT) '$$PWD'/$(MACHINE)/tmpos; \
			/etc/newfs /dev/r$(TMPOS_USR) "$(USR_DISKTYPE)"; \
			/etc/fsck /dev/r$(TMPOS_USR); \
			mkdir $(MACHINE)/tmpos/usr; \
			/etc/mount /dev/$(TMPOS_USR) '$$PWD'/$(MACHINE)/tmpos/usr; \
		  fi; exit 0'; \
		  cd $(MACHINE) && \
	  	    MACHINE=$(MACHINE) sh ../src/dist/proto.sh
clean install	:
