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

# $Header: Makefile 1.21 1991/06/11 17:41:02 $
#
TPFLAGS	= ../liberal
RFLAGS	= -e
REFERI	= rsh crg0 "refer -e -n"
STAGE	= $(STAGEROOT)/vol2/usr.doc
INSTDIR	= $(DESTDIR)/usr/doc
SRC	=\
	README awk bc dc implement iosys learn lex lint m4 make password\
	rogue security sed tbl trek
TARGETS	= $(STDTRGS) awk.F implement.F learn.F lint.F make.F rogue.F
STDTRGS	=\
	bc.F dc.F iosys.F lex.F m4.F password.F security.F\
	sed.F tbl.F trek.F
PRINT	=\
	awk.P bc.P dc.P implement.P iosys.P learn.P lex.P lint.P m4.P make.P\
	password.P rogue.P security.P sed.P tbl.P trek.P

DOCS =\
	cacm\
	ed\
	adv.ed\
	shell\
	ms\
	eqn\
	eqnguide\
	tbl\
	pub\
	troff\
	trofftut\
	lint\
	make\
	uprog\
	yacc\
	lex\
	m4\
	sed\
	awk\
	dc\
	bc\
	impl\
	iosys\
	uucp\
	uucpimp\
	secur\
	password\
	uchanges\
	csh\
	ex\
	Mail\
	rogue\
	me\
	diction\
	refer\
	curses\
	config\
	lpd\
	fsck\
	sendmail\
	sysman\
	fs\
	gprof\
	ipc\
	trek

all: $(DOCS)

$(DOCS): FRC
	./run $@
install		:
		install -c -m 444 $(SRC) cman $(INSTDIR)
		install -c -m 555 run $(INSTDIR)
		install -c -m 555 Makefile $(INSTDIR)

format		:& $(TARGETS)
$(STDTRGS)	:;$(TPF) -M$(TMACMSB) $(TPFLAGS) $* > $(STAGE)/$@;\
		rm -f $@; ln -s $(STAGE)/$@
awk.F		:awk; cat $* | $(REFER) | $(TPF) -M$(TMACMSB) $(TPFLAGS) - \
		> $(STAGE)/$@; rm -f $@; ln -s $(STAGE)/$@
bc.F		:bc
dc.F		:dc
implement.F	:implement;\
		cat $* | $(REFERI) | $(TPF) -M$(TMACMSB) $(TPFLAGS) - \
		> $(STAGE)/$@; rm -f $@; ln -s $(STAGE)/$@
iosys.F		:iosys
learn.F		:learn; $(TPF) -M$(TMACMS) $(TPFLAGS) $* > $(STAGE)/$@;\
		rm -f $@; ln -s $(STAGE)/$@
lex.F		:lex
lint.F		:lint; cat $* | $(REFER) | $(TPF) -M$(TMACMSB) $(TPFLAGS) - \
		> $(STAGE)/$@; rm -f $@; ln -s $(STAGE)/$@
m4.F		:m4
make.F		:make;\
		$(TPF) -M$(TMACMSB) $(TPFLAGS) ../picmacs $* > $(STAGE)/$@;\
		rm -f $@; ln -s $(STAGE)/$@
password.F	:password
rogue.F		:rogue; $(TPF) -M$(TMACME) -rv1 $(TPFLAGS) $* > $(STAGE)/$@;\
		rm -f $@; ln -s $(STAGE)/$@
security.F	:security
sed.F		:sed
tbl.F		:tbl
trek.F		:trek
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		:& $(PRINT)
clean		:; rm -f *.F $(STAGE)/*.F
FRC:
