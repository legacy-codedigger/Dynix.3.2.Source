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

# $Header: Makefile 1.20 89/11/06 $
#
ITPF	= /usr/local/bin/itpf -x
TPFLAGS	= man.macs

INSTDIR	= $(DESTDIR)/usr/man/man0

.SUFFIXES:
.SUFFIXES: .in .ti .xr .F .P .Y

SRC	=\
	bfile copyr.xr cshcmd ignore makeptx maketoc manuals.xr\
	ptxmac title1.ti title2.ti title4.ti toc.in xx

PRINT	= title1.P title2.P title4.P copyr.P manuals.P toc.P ptx.P

FINAL	= title1.Y title2.Y title4.Y copyr.Y manuals.Y toc.Y ptx.Y

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:; install -c -m 444 $(SRC) $(INSTDIR)
#	install -c -m 444 Makefile $(INSTDIR)/Makefile

ptxin	:makeptx; makeptx > $@

ptx.F	:ptxin cshcmd bfile ignore;\
	cat ptxin cshcmd | ptx -r -f -t -w 110 -b bfile -i ignore > $@

ptx.P		:ptx.F;$(ITPF) ../$(TPFLAGS) ptxmac ptx.F

.ti.P .xr.P	:;$(ITPF) ../$(TPFLAGS) $<

# warning: if this changes, add/delete other "install" entries and update maketoc.

TOCLIST	=\
	toc1 toc2 toc3 toc3m toc3n toc3p toc3s toc3x toc3c\
	toc4 toc5 toc6 toc7 toc8

toc.P		:tocs toc.in; $(ITPF) ../$(TPFLAGS) toc.in

tocs		:& $(TOCLIST)

$(TOCLIST)	:maketoc ptxin; maketoc $@ > $@

print	:& $(PRINT)
