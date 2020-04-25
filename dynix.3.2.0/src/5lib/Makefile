
##
# NOTE: libc is done by upper level Makefile
#       (and thus is not in SUBDIRS)
##

SUBDIRS	= libcurses libPW libl libm libmalloc liby libcrypt libplot libg
INSTALL	= libcurses.i libPW.i libl.i libm.i libmalloc.i liby.i libcrypt.i libplot.i libg.i

all:&	$(SUBDIRS)

$(SUBDIRS) libc: FRC
	cd $@; make $(MFLAGS) $(MRULES)

FRC:

install:& $(INSTALL)

$(INSTALL):
	cd `basename $@ .i`; make $(MFLAGS) $(MRULES) install

libc.install:
	cd libc; make $(MFLAGS) $(MRULES) install

clean clobber:
	@for i in $(SUBDIRS) libc; do \
		echo "	cd $$i; make $(MFLAGS) $(MRULES) $@"; \
	       (cd $$i; make $(MFLAGS) $(MRULES) $@); \
	done
