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

# $Header: Makefile 1.4 87/05/07 $
#
LIB	= -ldbm
SUBDIR	= revnetgroup
CFLAGS	= -O $(INCLUDE)
CMDS	= makedbm stdhosts yppoll yppush ypset ypxfr
SCRIPTS	= ypinit ypxfr_1perday ypxfr_2perday ypxfr_1perhour

all		:& $(SUBDIR) $(CMDS)
$(SUBDIR)	:FRC; cd $@ && make $(MFLAGS) $(MRULES)
$(CMDS)		:; $(CC) $(CFLAGS) -o $@ $@.c $(LIB)
install		:; install -s $(CMDS) $(NFS_DESTDIR)/usr/etc/yp
		   for i in $(SCRIPTS); do \
			install -c $$i.sh $(NFS_DESTDIR)/usr/etc/yp/$$i; done
		   for i in $(SUBDIR); do \
		  	(cd $$i && make $(MFLAGS) $(MRULES) install); done
		   install -c -m 644 make.script $(NFS_DESTDIR)/usr/etc/yp/Makefile
		   rm -rf $(NFS_DESTDIR)/etc/yp
		   ln -s /usr/etc/yp $(NFS_DESTDIR)/etc/yp
clean		:; rm -f $(CMDS) *.o && \
		   cd $(SUBDIR) && make $(MFLAGS) $(MRULES) clean
FRC		:
