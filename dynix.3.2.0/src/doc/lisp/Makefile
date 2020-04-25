# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.4 91/03/29 $
#
NROFF	= nroff
TPFLAGS	= ../liberal 
STAGE	= $(STAGEROOT)/vol2/lisp
INSTDIR	= $(DESTDIR)/usr/doc/lisp
LIBDIR	= $(DESTDIR)/usr/lib/lisp

ASRC	= ch0.n ch1.n ch2.n ch3.n ch4.n ch5.n ch6.n ch61.n ch7.n \
	  ch8.n ch9.n ch10.n ch11.n ch12.n ch13.n ch14.n ch15.n ch16.n  \
	  ch17.n chb.n chc.n

BSRC	= extrnames.awk fixmks.sed franz.n indexsed lmacs mantags

TFILES	= ch0.F ch1.F ch2.F ch3.F ch4.F ch5.F ch6.F \
  	  ch61.F ch7.F ch8.F ch9.F ch10.F ch11.F ch12.F \
	  ch13.F ch14.F ch15.F ch16.F ch17.F chb.F chc.F

XFILES	= ch0.x ch1.x ch2.x ch3.x ch4.x ch5.x ch6.x \
  	  ch61.x ch7.x ch8.x ch9.x ch10.x ch11.x ch12.x \
	  ch13.x ch14.x ch15.x ch16.x ch17.x chb.x chc.x

RFILES	= ch0.r ch1.r ch2.r ch3.r ch4.r ch5.r ch6.r \
  	  ch61.r ch7.r ch8.r ch9.r ch10.r ch11.r ch12.r \
	  ch13.r ch14.r ch15.r ch16.r ch17.r chb.r chc.r

RXFILES	= ch0.rx ch1.rx ch2.rx ch3.rx ch4.rx ch5.rx ch6.rx \
  	  ch61.rx ch7.rx ch8.rx ch9.rx ch10.rx ch11.rx ch12.rx \
	  ch13.rx ch14.rx ch15.rx ch16.rx ch17.rx chb.rx chc.rx

PRINT	=

# sources: lmacs: macros for the franz documents
#	   ch1.n  intro and description of types
#	   ch2.n  data structure access
#	   ch3.n  on arithmetic functions.
#	   ch4.n  special functions.
#	   ch5.n  i/o
#	   ch6.n  system functions
#	   ch7.n  reader
#	   ch8.n  functions and macros 
#	   ch9.n  arrays 
#	   ch10.n exception handling 
#	   ch11.n trace package 
#	   ch12.n liszt 
#	   ch13.n cmu top level
#	   ch14.n stepper 
#	   ch15.n fixit package
#	   ch16.n lisp editor
#	   chb.n special symbols
#	   chc.n  short subjects

.SUFFIXES: .n .F .r

# the syntax's have this meaning:

#  .n  nroff/troff source file
#  .F  TPF output file
#  .x  index file from a troff run, used to produce the index
#  .r  nroff output file.
#  .rx  special index output from nroff run.  used to produce the helpindex

.n.F:;	$(TPF) -M$(TMACME) $(TPFLAGS) lmacs $*.n 1> $(STAGE)/$*.F 2> $*.x; \
		rm -f $*.F; ln -s $(STAGE)/$*.F

.n.r:;	tbl lmacs $*.n | ${NROFF} -rb3 -me 1> $*.r 2> $*.rx

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

# make install will install the nroff versions of the manual in the
# directory (LIBDIR/manual) where the auxfns0.l help command can find them.

install:&	$(RFILES)
	if [ "$(MACHINE)" = "ns32000" ]; then \
		cat $(RXFILES) | tr '\227' ' ' > helpindex;\
		install -m 444 $(RFILES) helpindex ${LIBDIR}/manual;\
		install -c -m 444 $(ASRC) $(BSRC) $(INSTDIR);\
		install -c -m 444 Makefile.proto $(INSTDIR)/Makefile;\
	else\
		echo "Not installing lisp documentation on $(MACHINE).";\
	fi

format:		parallel index.F

parallel:&	$(TFILES)

index.F:&	$(TFILES)
	(echo ".Ib"; sort +3 $(XFILES)) | sed -f indexsed  > indexx
	$(TPF) -M$(TMACME) $(TPFLAGS) lmacs indexx > $(STAGE)/index.F
	rm -f index.F; ln -s $(STAGE)/index.F

clean:
	-rm -f *.r *.rx *.F *.x helpindex indexx index $(STAGE)/*.F

mkstage:
	-mkdir $(STAGEROOT)/vol2 $(STAGE)

print:		format
	$(LPR) $(TFILES) index.F
