
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

LEV1 =	graphics
LEV2 =	glib stat dev toc gutil
DOC =	whatis

all:
	make $(MFLAGS) $(MRULES) $(LEV1) $(LEV2) $(DOC)

install:
	install -c -m 755 graphics $(DESTDIR)/usr/bin
	install -c -m 666 /dev/null $(DESTDIR)/usr/adm/graflog
	install -c -m 755 whatis $(DESTDIR)/usr/bin/whatis
	for i in $(LEV2); do\
		(cd $$i.d; make $(MFLAGS) $(MRULES) install); \
	done
	cd $(DOC).d; make $(MFLAGS) $(MRULES) install

graphics:
	cp graphics.sh graphics;

$(LEV2):
	cd $@.d; make $(MFLAGS) $(MRULES)

$(DOC):
	cp $@.sh $@;

clean clobber:
	-rm -f $(DOC) $(LEV1)
	for i in $(LEV2); do\
		(cd $$i.d; make $(MFLAGS) $(MRULES) clean);\
	done
	cd $(DOC).d; make $(MFLAGS) $(MRULES) clean
