
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
INSDIR = $(DESTDIR)/usr/bin
OFILES = r0.o r1.o r2.o rio.o rlook.o rlex.o

all: ratfor

ratfor:    $(OFILES) r.g.o
	   $(CC) $(OFILES) r.g.o -o ratfor -ly

$(OFILES): r.h r.g.h
r.g.c r.g.h:   r.g.y
	   $(YACC) -d r.g.y
	mv y.tab.c r.g.c
	mv y.tab.h r.g.h

install:
	install -c ratfor $(INSDIR)/ratfor

clean clobber:
	   -rm -f *.o
	   -rm -f r.g.c r.g.h ratfor
