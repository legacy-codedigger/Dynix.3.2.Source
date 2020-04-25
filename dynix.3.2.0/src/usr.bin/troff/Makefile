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

# $Header: Makefile 2.2 86/03/23 $
#
# Common makefile for nroff and troff
#
CFLAGS	= -O -DVMUNIX $(INCLUDE)
#

MISC	= tmac fontinfo font term ms me/src
LINKS	= tabtn300.ascii tabcrt.ascii tab300s.ascii tab300s-12.ascii \
	  tabdtc.ascii tabdtc12.ascii tabipsi.ascii tabipsi12.ascii
OBJS	= ni.o nii.o n1.o n2.o n3.o n4.o n5.o n7.o n8.o n9.o hytab.o suftab.o \
	  tab3.o

.c.o:
	$(CC) $(CFLAGS) -UNROFF -c $*.c & \
	cd n && \
	$(CC) $(CFLAGS) -DNROFF -c ../$*.c & \
	wait

all		:  n nroff troff
n		:; mkdir n
nroff		:& $(OBJS) n6.o n10.o
		   cd n && \
		   $(CC) -o ../nroff $(OBJS) n6.o n10.o
troff		:& $(OBJS) t6.o t10.o
		   $(CC) -o troff $(OBJS) t6.o t10.o

# Code not shared won't compile without correct defines
n6.o		:  n6.c; cd n && $(CC) $(CFLAGS) -DNROFF -c ../$*.c && cp n6.o ..
n10.o		:  n10.c; cd n && $(CC) $(CFLAGS) -DNROFF -c ../$*.c && cp n10.o ..
t6.o		:  t6.c; $(CC) $(CFLAGS) -UNROFF -c $*.c
t10.o		:  t10.c; $(CC) $(CFLAGS) -UNROFF -c $*.c
hytab.o		:  hytab.c; $(CC) -R -c hytab.c && cp $*.o n/$*.o
suftab.o	:  suftab.c; $(CC) -R -c suftab.c && cp $*.o n/$*.o
install		:& ivfont iobjs imisc
		  -cd $(DESTDIR)/usr/lib/vfont && \
		   chown root . * && chgrp daemon . *  2>/dev/null
ivfont		:; uncompress < vfont/VFONT.tar.Z | \
		   (cd $(DESTDIR)/usr/lib; tar xpBf -)
iobjs		:; install -s nroff troff $(DESTDIR)/usr/bin
imisc		:& $(MISC) me mm
		   cd $(DESTDIR)/usr/lib/ms && chmod 755 swapacc endnote
		   chmod 755 $(DESTDIR)/usr/lib/me/src/install
		   cd $(DESTDIR)/usr/lib/term && \
		   	rm -f $(LINKS) && \
		   	ln tablpr.ascii tabtn300.ascii && \
		   	ln tablpr.ascii tabcrt.ascii && \
		   	ln tab302.ascii tab300s.ascii && \
		   	ln tab302-12.ascii tab300s-12.ascii && \
		   	ln tab302.ascii tabdtc.ascii && \
		   	ln tab302-12.ascii tabdtc12.ascii && \
		   	ln tab450.ascii tabipsi.ascii && \
		   	ln tab450-12.ascii tabipsi12.ascii
$(MISC)		:FRC; cd $@ && install -c -m 644 * $(DESTDIR)/usr/lib/$@
me		:FRC; cd me && \
		   install -c -m 644 *.me revisions $(DESTDIR)/usr/lib/me
mm		:FRC; cd mm && \
		   install -c -m 644 tmac.m $(DESTDIR)/usr/lib/tmac/tmac.m && \
		   install -c -m 644 mmn.src $(DESTDIR)/usr/lib/mm/mmn && \
		   install -c -m 644 mmt.src $(DESTDIR)/usr/lib/mm/mmt
clean		:; rm -rf n $(OBJS) t6.o n6.o t10.o n10.o nroff troff
FRC		:
