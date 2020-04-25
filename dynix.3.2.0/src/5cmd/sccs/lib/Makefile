
SLIB	= cassi.a comobj.a mpwlib.a libPW.a
SUBDIR	= cassi comobj mpwlib libPW

all:& 	$(SUBDIR)

$(SUBDIR): FRC
	cd $@; make $(MFLAGS) $(MRULES)

FRC:

install:

clean clobber:
	@for i in $(SUBDIR); do \
		echo "	cd $$i; make $(MFLAGS) $(MRULES) $@"; \
		       (cd $$i; make $(MFLAGS) $(MRULES) $@); \
	done
	rm -f $(SLIB) core *.o
