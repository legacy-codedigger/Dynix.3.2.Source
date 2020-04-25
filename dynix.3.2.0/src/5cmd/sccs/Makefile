
SUBDIRS	= lib cmd

all:	$(SUBDIRS)

$(SUBDIRS): FRC
	cd $@; make $(MFLAGS) $(MRULES)

FRC:

install:
	cd cmd; make $(MFLAGS) $(MRULES) install
	cd help.d; make $(MFLAGS) $(MRULES) install

clean clobber:
	cd lib; make $(MFLAGS) $(MRULES) $@
	cd cmd; make $(MFLAGS) $(MRULES) $@
