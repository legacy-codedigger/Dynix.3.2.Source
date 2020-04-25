
DESTDIR	= $(ATT_DESTDIR)
DIRS	=\
	$(DESTDIR)/usr/catman\
	$(DESTDIR)/usr/catman/a_man\
	$(DESTDIR)/usr/catman/a_man/man1\
	$(DESTDIR)/usr/catman/a_man/man7\
	$(DESTDIR)/usr/catman/a_man/man8\
	$(DESTDIR)/usr/catman/p_man\
	$(DESTDIR)/usr/catman/p_man/man2\
	$(DESTDIR)/usr/catman/p_man/man3\
	$(DESTDIR)/usr/catman/p_man/man4\
	$(DESTDIR)/usr/catman/p_man/man5\
	$(DESTDIR)/usr/catman/u_man\
	$(DESTDIR)/usr/catman/u_man/man1\
	$(DESTDIR)/usr/catman/u_man/man6
FILES	=\
	a_man1\
	a_man7\
	a_man8\
	p_man2\
	p_man3\
	p_man4\
	p_man5\
	u_man1\
	u_man6

all:

# BTW: directories can not be done in parallel due to subdirs
install: $(DIRS) files
	install -c -m 755 man.sh $(DESTDIR)/usr/bin/man
	install -c -m 444 notes $(DESTDIR)/usr/catman/notes
	install -c -m 644 term $(DESTDIR)/usr/lib/help/term

$(DIRS):; mkdir $@; chmod 755 $@; chgrp daemon $@; sh -c 'chown root $@; exit 0'

files:& $(FILES)

a_man1: ; cd catman/a_man/man1; install -c -m 444 * $(DESTDIR)/usr/catman/a_man/man1/
a_man7: ; cd catman/a_man/man7; install -c -m 444 * $(DESTDIR)/usr/catman/a_man/man7/
a_man8: ; cd catman/a_man/man8; install -c -m 444 * $(DESTDIR)/usr/catman/a_man/man8/
p_man2: ; cd catman/p_man/man2; install -c -m 444 * $(DESTDIR)/usr/catman/p_man/man2/
p_man3: ; cd catman/p_man/man3; install -c -m 444 * $(DESTDIR)/usr/catman/p_man/man3/
p_man4: ; cd catman/p_man/man4; install -c -m 444 * $(DESTDIR)/usr/catman/p_man/man4/
p_man5: ; cd catman/p_man/man5; install -c -m 444 * $(DESTDIR)/usr/catman/p_man/man5/
u_man1: ; cd catman/u_man/man1; install -c -m 444 * $(DESTDIR)/usr/catman/u_man/man1/
u_man6: ; cd catman/u_man/man6; install -c -m 444 * $(DESTDIR)/usr/catman/u_man/man6/

clean clobber:
