
SUBDIRS	= misc vax

all: $(SUBDIRS)

$(SUBDIRS): FRC
	cd $@; make $(MFLAGS) $(MRULES)

FRC:

install:
	cd vax; make $(MFLAGS) $(MRULES) install 

clean clobber:
	cd misc; make $(MFLAGS) $(MRULES) $@
	cd vax; make $(MFLAGS) $(MRULES) $@
