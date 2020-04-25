
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)

all:
	:

install:
	-sh -c 'for i in *.w; do\
   		NAME=`echo $$i | sed "s/\.w//"`; \
   		cp $$NAME.w $$NAME; \
   		install -m 644 $$NAME $(DESTDIR)/usr/lib/graf/whatis/$$NAME; \
	done'

clean clobber:
	:
