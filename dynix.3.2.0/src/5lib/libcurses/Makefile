all:
	cd screen; make $(MFLAGS) $(MRULES)

install clean clobber:
	cd screen; make $(MFLAGS) $(MRULES) $@
	cd terminfo; make $(MFLAGS) $(MRULES) $@
