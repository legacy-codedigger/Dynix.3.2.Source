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


# $Header: Makefile 3.9 85/08/02 $

# Copyright (C) 1982 by Walter F. Tichy
#                       Purdue University
#                       Computer Science Department
#                       West Lafayette, IN 47907
#
# All rights reserved. No part of this software may be sold or distributed
# in any form or by any means without the prior written permission of the
# author.
# Report problems and direct all inquiries to
# tichy@purdue, tichy@purdue-tcp, decvax!pur-ee!purdue!tichy


#               INSTRUCTIONS
#               ============
#
# 0.  Do the instructions in rcsbase.h
#
# 1.  Figure out where to put the modified diff and diff3 commands, and
#     install them. (Read the instructions in the Make-files).
#     Define the macros DIFF and DIFF3 accordingly (below).
DIFF      = /usr/lib/rdiff
DIFF3     = /usr/lib/rdiff3

# 2.  Implant the macro DIFF3 as modified by you into the file merge.
#     (That's a shell file.)

# 3.  Figure out where to put the RCS commands; define TARGETDIR
#     and merge accordingly.
TARGETDIR = /usr/bin
MERGE     = /usr/bin/merge

# 4.  Figure out where to put your old RCS commands; define OLDDIR
#     accordingly. This is not needed if you don't have an old release of RCS.
#     Instead, comment out the 5 lines in INSTALLNO that mv the old RCS
#     commands to OLDDIR.
# OLDDIR    = /usr/old

# 5.  Decide what loader flags you need, and define LDFLAGS accordingly.
#     On the Vax you need none, on PDP-11s with split i/d you need -i.
LDFLAGS   =
#LDFLAGS  = -i

# 6.  RCS can maintain a log of the use of the co and ci commands. If you
#     want the logging, you must
#     a) define SNOOP (that's where the process that writes the log goes),
#     b) define SNOOPFILE (that's the file where SNOOP writes the log; it must
#        be owned by the same user that owns SNOOP),
#     c) include SNOOP and SNOOPFILE in the CFLAGS macro.
#     If you don't want the logging of command usage, omit the SNOOP and
#     SNOOPFILE definitions from CFLAGS.
#     The two variants of the CFLAG macros are given below.
SNOOPFILE = /usr/wft/RCSLOG/log
SNOOP     = /usr/wft/BIN/snoop
#CFLAGS    = -O -DCOMPAT2 -DDIFF='"$(DIFF)"' -DMERGE='"$(MERGE)"' -DTARGETDIR='"$(TARGETDIR)"' -DSNOOPFILE='"$(SNOOPFILE)"' -DSNOOP='"$(SNOOP)"'
CFLAGS   = -O -DDIFF='"$(DIFF)"' -DMERGE='"$(MERGE)"' -DTARGETDIR='"$(TARGETDIR)"' -DV4_2BSD ${INCLUDE}

# 7.  If you don't have release 2 RCS files around (e.g., if you don't have
#     an old release of RCS), remove the word -DCOMPAT2 from whatever
#     variant of CFLAGS you selected. -DCOMPAT2 assures that release 2
#     RCS files can still be read. (After all release 2 RCS files have
#     been updated with release 3 ci or rcs, you can remake RCS without
#     -DCOMPAT2.) Also, make sure that the co commands at the end of this
#     file are commented out; otherwise, Make will complain.
#
# 8.  Now you are ready. Try to make all. If all went well, make
#     INSTALLW if you want the logging of command usage, INSTALLNO otherwise.
#
# 9.  IMPORTANT! When you have RCS installed, rename old RCS files as
#     follows (if you have any):
#     If the working file was "f.c" and the RCS file "f.c.v", rename the
#     RCS file to "f.c,v". If the working file was "f.c" and the RCS file
#     "f.v", rename the RCS file "f.c,v". Thus, suffixes are no longer
#     dropped and RCS files end in ",v" rather than ".v".
#
# 10. If you want to maintain RCS with itself, be sure you preserve the
#     original revision numbers, dates, etc. This is done by checking the
#     files in with the -k option. When done, remove the comments from the
#     co-rules at the end of this file.

OBJECTS = rcslex.o rcssyn.o rcsrev.o rcsgen.o rcssynTST.o rcsrevTST.o\
	rcsedit.o rcsutil.o rcsfnms.o co.o ci.o ident.o rlog.o rcs.o\
	rcsdiff.o rcsmerge.o rcskeep.o rcsfcmp.o snoop.o
RCSCOMMANDS = ci ident rcs rcsdiff rcsmerge rlog co

all:		merge $(RCSCOMMANDS)

INSTALLW:	INSTALLSNOOP INSTALLNO

install:	INSTALLNO
 
INSTALLNO:	merge $(RCSCOMMANDS)
		install -m 755 -c merge ${DESTDIR}/${MERGE}
		for i in ${RCSCOMMANDS}; do \
			install -s -m 755 $$i ${DESTDIR}/${TARGETDIR}/$$i; \
		done

INSTALLSNOOP:	snoop
		${STRIP} snoop
		mv snoop $(SNOOP)
		chmod u+s $(SNOOP)
		chmod 644 $(SNOOPFILE)
# CAUTION: You may have to do a chown of SNOOP and SNOOPFILE.

clean:		; rm -f *.o $(RCSCOMMANDS) snoop

depend:;

$(OBJECTS):	rcsbase.h

partime.o maketime.o co.o ci.o rlog.o rcsutil.o:	time.h

 CI = ci.o rcslex.o rcssyn.o rcsgen.o rcsedit.o rcsrev.o rcsutil.o rcsfnms.o rcskeep.o rcsfcmp.o
ci:		$(CI); ${CC} $(LDFLAGS) -o ci $(CI)

CO = co.o rcslex.o rcssyn.o rcsgen.o rcsedit.o rcsrev.o rcsutil.o rcsfnms.o partime.o maketime.o
co:		$(CO); ${CC} $(LDFLAGS) -o co $(CO)

ident:		ident.o
		${CC} -o ident ident.o

RLOG = rlog.o rcslex.o rcssyn.o rcsrev.o rcsutil.o partime.o maketime.o rcsfnms.o
rlog:		$(RLOG); ${CC} $(LDFLAGS) -o rlog $(RLOG)

RCS = rcs.o rcslex.o rcssyn.o rcsrev.o rcsutil.o rcsgen.o rcsedit.o rcsfnms.o
rcs:		$(RCS); ${CC} $(LDFLAGS) -o rcs $(RCS)

RCSDIFF = rcsdiff.o rcsutil.o rcsfnms.o rcsrev.o rcssyn.o rcslex.o
rcsdiff:	$(RCSDIFF); ${CC} -o rcsdiff $(RCSDIFF)

RCSMERGE = rcsmerge.o rcsutil.o rcsfnms.o rcsrev.o rcssyn.o rcslex.o
rcsmerge:	$(RCSMERGE); ${CC} -o rcsmerge $(RCSMERGE)

snoop:		snoop.o
		${CC} -o snoop snoop.o
