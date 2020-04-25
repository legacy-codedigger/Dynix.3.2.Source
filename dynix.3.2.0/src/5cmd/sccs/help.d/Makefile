
DESTDIR	= $(ATT_DESTDIR)
HELPLIB = $(DESTDIR)/usr/lib/help
I	= install -c -m 644 $? $@
IFILES	=\
	$(HELPLIB)/ad\
	$(HELPLIB)/bd\
	$(HELPLIB)/cb\
	$(HELPLIB)/cm\
	$(HELPLIB)/cmds\
	$(HELPLIB)/co\
	$(HELPLIB)/de\
	$(HELPLIB)/default\
	$(HELPLIB)/ge\
	$(HELPLIB)/he\
	$(HELPLIB)/prs\
	$(HELPLIB)/rc\
	$(HELPLIB)/un\
	$(HELPLIB)/ut\
	$(HELPLIB)/vc

all:

install:& $(IFILES)

$(HELPLIB)/ad: ad; $(I)
$(HELPLIB)/bd: bd; $(I)
$(HELPLIB)/cb: cb; $(I)
$(HELPLIB)/cm: cm; $(I)
$(HELPLIB)/cmds: cmds; $(I)
$(HELPLIB)/co: co; $(I)
$(HELPLIB)/de: de; $(I)
$(HELPLIB)/default: default; $(I)
$(HELPLIB)/ge: ge; $(I)
$(HELPLIB)/he: he; $(I)
$(HELPLIB)/prs: prs; $(I)
$(HELPLIB)/rc: rc; $(I)
$(HELPLIB)/un: un; $(I)
$(HELPLIB)/ut: ut; $(I)
$(HELPLIB)/vc: vc; $(I)

clean clobber:
