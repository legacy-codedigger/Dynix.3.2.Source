
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
OWN	= lp
GRP	= daemon
IDIR	= $(DESTDIR)/usr/spool/lp/model
SOURCE	= 1640 dumb f450 hp prx pprx ph.daps PRIME3260p PRIME3260s PRIME3280p \
	  PRIME3450p PRIME3450s

all:	$(SOURCE)
	:

install:
	for i in $(SOURCE); do \
		install -c -m 755 -o $(OWN) -g $(GRP) $$i $(IDIR)/$$i; \
	done

clean clobber:	; :
