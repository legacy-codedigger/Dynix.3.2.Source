#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)subndx:subndx.mk	1.7"
#	subj/ndx make file (text subsystem)
#
# DSL 2
OL = $(ROOT)/
INS = :
ARGS = all
INSDIR = $(OL)usr/bin
INSLIB = $(OL)usr/lib
LDFLAGS = -s
MAKE = make

compile all:	subj ndx

subj:	makedir sbj1 sbj2 sbj3 parts
	cp subj.sh subj
	$(INS) subj $(INSDIR)
	cd $(INSDIR); chmod 775 subj; $(CH) chgrp bin subj; chown bin subj


sbj1:	sbj1.o cnst.h
	$(CC) -O $(LDFLAGS) sbj1.o -ll -o sbj1
	$(INS) sbj1 $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 755 sbj1; $(CH) chgrp bin sbj1; chown bin sbj1

sbj2:	sbj2.o case.o cnst.h
	$(CC) -O $(LDFLAGS) sbj2.o case.o -ll -o sbj2
	$(INS) sbj2 $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 755 sbj2; $(CH) chgrp bin sbj2; chown bin sbj2

sbj3:	sbj3.o case.o omit.o cnst.h
	$(CC) -O $(LDFLAGS) sbj3.o case.o omit.o -ll -o sbj3
	$(INS) sbj3 $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 755 sbj3; $(CH) chgrp bin sbj3; chown bin sbj3



ndx:	makedir ndexer pages ndxformat sbjprep
	cp ndx.sh ndx
	$(INS) ndx $(INSDIR)
	cd $(INSDIR); chmod 775 ndx; $(CH) chgrp bin ndx; chown bin ndx

ndexer:	ndexer.o rootwd.o str.o strr.o case.o space.o dstructs.h ehash.h edict.h
	$(CC) -O $(LDFLAGS) ndexer.o rootwd.o str.o strr.o case.o space.o -ll -i -o ndexer
	$(INS) ndexer $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 755 ndexer; $(CH) chgrp bin ndexer; chown bin ndexer

pages:	pages.c
	$(CC) -O $(LDFLAGS) pages.c -o pages
	$(INS) pages $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 755 pages; $(CH) chgrp bin pages; chown bin pages

ndxformat:	ndxformat.c
		$(CC) -O $(LDFLAGS) ndxformat.c -o ndxformat
		$(INS) ndxformat $(INSLIB)/dwb
		cd $(INSLIB)/dwb; chmod 755 ndxformat; $(CH) chgrp bin ndxformat; \
		chown bin ndxformat

sbjprep:	sbjprep.c
		$(CC) -O $(LDFLAGS) sbjprep.c -o sbjprep
		$(INS) sbjprep $(INSLIB)/dwb
		cd $(INSLIB)/dwb; chmod 755 sbjprep; $(CH) chgrp bin sbjprep;\
		chown bin sbjprep



parts:	parts.sh style1 style2 style3 deroff
	cp parts.sh parts
	$(INS) parts $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 775 parts; $(CH) chgrp bin parts; chown bin parts

style1:	nwords.o nhash.h dict.h ydict.h names.h abbrev.h
	$(CC) -O $(LDFLAGS) nwords.o -ll -o style1
	$(INS) style1 $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 755 style1; $(CH) chgrp bin style1; chown bin style1

style2:	end.o ehash.h edict.h names.h
	$(CC) -O $(LDFLAGS) end.o -ll -o style2
	$(INS) style2 $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 755 style2; $(CH) chgrp bin style2; chown bin style2

style3:	part.o pscan.o outp.o extern.o
	$(CC) -O $(LDFLAGS) part.o pscan.o outp.o extern.o -ll -o style3
	$(INS) style3 $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 755 style3; $(CH) chgrp bin style3; chown bin style3

deroff:	deroff.o
	$(CC) -O $(LDFLAGS) deroff.o -i -o deroff
	$(INS) deroff $(INSLIB)/dwb
	cd $(INSLIB)/dwb; chmod 755 deroff; $(CH) chgrp bin deroff; chown bin deroff

makedir:
		if [ ! -d $(INSLIB)/dwb ] ; then rm -f $(INSLIB)/dwb; \
			mkdir $(INSLIB)/dwb;  chmod 755 $(INSLIB)/dwb;  fi


install:
	$(MAKE) -f subndx.mk INS=cp ROOT=$(ROOT) CH=$(CH) $(ARGS)
inssubj:	;  $(MAKE) -f subndx.mk INS=cp ROOT=$(ROOT) CH=$(CH) subj
insndx:	;  $(MAKE) -f subndx.mk INS=cp ROOT=$(ROOT) CH=$(CH) ndx

clean:	subjclean ndxclean

subjclean:	
	rm -f sbj1.o sbj2.o sbj3.o case.o omit.o end.o nwords.o part.o \					pscan.o outp.o extern.o deroff.o

ndxclean:	
	rm -f ndexer.o rootwd.o str.o strr.o case.o space.o pages.o \					 ndxformat.o sbjprep.o

clobber:	clean subjclobber ndxclobber

subjclobber:	
	rm -f sbj1 sbj2 sbj3 subj parts style1 style2 style3 deroff

ndxclobber:	
	rm -f ndx ndexer pages ndxformat sbjprep
