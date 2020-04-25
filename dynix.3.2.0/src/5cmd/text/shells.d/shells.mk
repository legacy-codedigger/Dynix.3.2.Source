#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)shells:shells.mk	1.36"
#	text subsystem shells make file
#
# DSL 2.

OL = $(ROOT)/
INS = :
INSDIR = $(OL)usr/bin
INSLIB = $(OL)usr/lib
HINSDIR = $(OL)usr/pub
FILES =  mm.sh mmt.sh 
SFILES = nroff.letter mm.report mm.sales mm.letter tbl.language tbl.bridges \
	tbl.pres eqn.stats troff.fonts troff.sizes troff.ad \
	troff.aeneid pic.forms
LDFLAGS = -s
MAKE = make

compile all:  mm mmt mvt termh samples
	:

mm:	mm.sh
	cp mm.sh mm
	$(INS) mm $(INSDIR)
	cd $(INSDIR); chmod 755 mm; $(CH) chgrp bin mm; chown bin mm

mmt:	mmt.sh
	cp mmt.sh mmt
	$(INS) mmt $(INSDIR)
	cd $(INSDIR); chmod 755 mmt; $(CH) chgrp bin mmt; chown bin mmt

mvt:	mmt
	rm -f $(INSDIR)/mvt
	ln $(INSDIR)/mmt $(INSDIR)/mvt
	cd $(INSDIR); chmod 755 mvt; $(CH) chgrp bin mvt; chown bin mvt

helpdir:
	if [ ! -d $(HINSDIR) ] ; then rm -f $(HINSDIR);  \
		mkdir $(HINSDIR);  chmod 755 $(HINSDIR);  fi

termh:	helpdir
	${INS} terminals ${HINSDIR}
	cd ${HINSDIR}; chmod 664 terminals; $(CH) chgrp bin terminals; chown bin terminals

samples: $(SFILES)
	if [ ! -d $(INSLIB)/dwb ] ; then rm -f $(INSLIB)/dwb;  \
		mkdir $(INSLIB)/dwb;  chmod 755 $(INSLIB)/dwb;  fi
	if [ ! -d $(INSLIB)/dwb/samples ] ;  \
		then rm -f $(INSLIB)/dwb/samples; \
		mkdir $(INSLIB)/dwb/samples;  chmod 755 $(INSLIB)/dwb/samples;  fi
	$(INS) $(SFILES) $(INSLIB)/dwb/samples 
	cd $(INSLIB)/dwb/samples; chmod 644 $(SFILES); \
		$(CH) chgrp bin $(SFILES); chown bin $(SFILES)

install:
	$(MAKE) -f shells.mk INS=cp ROOT=$(ROOT) CH=$(CH)
insmm:	;  $(MAKE) -f shells.mk INS=cp ROOT=$(ROOT) CH=$(CH) mm
insmmt:	;  $(MAKE) -f shells.mk INS=cp ROOT=$(ROOT) CH=$(CH) mmt
insmvt:	;  $(MAKE) -f shells.mk INS=cp ROOT=$(ROOT) CH=$(CH) mvt
inssamples:	;	$(MAKE) -f shells.mk INS=cp ROOT=$(ROOT) CH=$(CH) samples

clean mmclean mmtclean mvtclean:
clobber:  clean mmclobber mmtclobber
	:
mmclobber:   ;  rm -f mm
mmtclobber mvtclobber:  ;  rm -f mmt
