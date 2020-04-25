#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)text:text.mk	1.32"
#	text sub-system make file
#
# DSL 2.

OL = $(ROOT)/
ARGS = all
INSDIR = $(OL)usr/bin
LDFLAGS = -s
INS = :
MAKE = make

compile all:	 notices roff macros shells eqn neqn tbl checkmm pic macref \
		 ptx grap subndx diffmk hyphen
	:

notices:	;	@echo ""
	@echo ""
	@echo ""
	@echo "			Copyright (c) 1984 AT&T"
	@echo "			  All Rights Reserved"
	@echo ""
	@echo "   THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T"
	@echo "The copyright notice above does not evidence any actual or"
	@echo "intended publication of such source code."
	@echo ""
	@echo ""
	@echo""


roff:	;  cd roff.d;  $(MAKE) -f roff.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

macros:	;  cd macros.d;  $(MAKE) -f macros.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

shells:	;  cd shells.d;  $(MAKE) -f shells.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

eqn:	;  cd eqn.d;  $(MAKE) -f eqn.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

neqn:	;  cd neqn.d; $(MAKE) -f neqn.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

tbl:	;  cd tbl.d;  $(MAKE) -f tbl.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

checkmm: ;  cd checkmm.d; $(MAKE) -f checkmm.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

pic: ;  cd pic.d; $(MAKE) -f pic.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

macref: ;  cd macref.d; $(MAKE) -f macref.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

ptx:	;  cd ptx.d; $(MAKE) -f ptx.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

grap:	;  cd grap.d; $(MAKE) -f grap.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

subndx:	;  cd subndx.d; $(MAKE) -f subndx.mk ROOT=$(ROOT) CH=$(CH) $(ARGS)

diffmk:	diffmk.sh
	cp diffmk.sh diffmk
	$(INS) diffmk $(INSDIR)
	cd $(INSDIR); chmod 775 diffmk; $(CH) chgrp bin diffmk; chown bin diffmk

hyphen:	hyphen.c
	$(CC) -O $(LDFLAGS) hyphen.c -o hyphen
	$(INS) hyphen $(INSDIR)
	cd $(INSDIR); chmod 775 hyphen; $(CH) chgrp bin hyphen; chown bin hyphen

install:
	$(MAKE) -f text.mk ARGS=install ROOT=$(ROOT) CH=$(CH) INS=cp $(ARGS)

clean:
	cd roff.d;  $(MAKE) -f roff.mk clean
	cd eqn.d;   $(MAKE) -f eqn.mk clean
	cd neqn.d;  $(MAKE) -f neqn.mk clean
	cd tbl.d;   $(MAKE) -f tbl.mk clean
	cd macros.d; $(MAKE) -f macros.mk clean
	cd shells.d; $(MAKE) -f shells.mk clean
	cd checkmm.d; $(MAKE) -f checkmm.mk clean
	cd pic.d; $(MAKE) -f pic.mk clean
	cd macref.d; $(MAKE) -f macref.mk clean
	cd ptx.d; $(MAKE) -f ptx.mk clean
	cd grap.d; $(MAKE) -f grap.mk clean
	cd subndx.d; $(MAKE) -f subndx.mk clean
	rm -f hyphen.o

clobber:
	cd roff.d;  $(MAKE) -f roff.mk clobber
	cd eqn.d;   $(MAKE) -f eqn.mk clobber
	cd neqn.d;  $(MAKE) -f neqn.mk clobber
	cd tbl.d;   $(MAKE) -f tbl.mk clobber
	cd macros.d;  $(MAKE) -f macros.mk clobber
	cd shells.d;  $(MAKE) -f shells.mk clobber
	cd checkmm.d; $(MAKE) -f checkmm.mk clobber
	cd pic.d; $(MAKE) -f pic.mk clobber
	cd macref.d; $(MAKE) -f macref.mk clobber
	cd ptx.d; $(MAKE) -f ptx.mk clobber
	cd grap.d; $(MAKE) -f grap.mk clobber
	cd subndx.d; $(MAKE) -f subndx.mk clobber
	rm -f hyphen diffmk

