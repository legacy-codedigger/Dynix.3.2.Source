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

# $Header: Makefile 1.15 90/02/28 $
#
TPFLAGS	= ../man.macs
INSTDIR	= $(DESTDIR)/usr/doc/misc
IMAGEN	= /usr/imagen
ITPFDIR	= $(IMAGEN)/bin
ITPF	= $(ITPFDIR)/itpf -x
IMAC	= $(IMAGEN)/lib/tmac
TMAC	= /usr/lib/tmac
TMACMS	= $(TMAC)/tmac.s
TOOLS	= /ccs/techpubs/tools
TMACTI = $(TOOLS)/tmac.ti
TMACTPF	= $(IMAC)/tmac.tpf

SRC	= contents.ms copyr.xr manuals.t title.ti

PRINT	= contents.P copyr.P manuals.P title.P

.SUFFIXES:
.SUFFIXES: .ms .t .ti .xr .P

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:; install -c -m 444 $(SRC) $(INSTDIR)

.ms.P	:;
	$(ITPF) -M$(TMACMS) ../man.macs $<

.t.P	:;
	$(ITPF) -M$(TMACTPF) ../man.macs $<

.ti.P	:;
	$(ITPF) -M$(TMACTI) ../man.macs $<

.xr.P	:;
	$(ITPF) -M ../man.macs $<


# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools $(PRINT)

# "make vlog" records the version numbers of all the files used in
# building this manual, including the standard tools.
vlog	:
	mkvlog -p "$(PARTNUMBER)" $(SRC) > versionlog
# If you have source files in subdirectories, include a line like the
# following one for each subdirectory:
#	cd subdirectory; make $(MFLAGS) $(MRULES) vlog
# That subdirectory's vlog target should be:
#	mkvlog $(SRC) >> ../versionlog
# Get the version numbers of all the tools:
	mkvlog -t >> versionlog
