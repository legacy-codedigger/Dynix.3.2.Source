
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
OBJS	= libmai.o libzer.o

all:	liby.a

liby.a:& $(OBJS)
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 $(OBJS)
	@-chgrp daemon $(OBJS) > /dev/null 2>&1
	@-chown  root $(OBJS) > /dev/null 2>&1
	@echo "	building $@"
	@$(AR) rcu liby.a $(OBJS)

install:
	install -m 644 liby.a $(DESTDIR)/usr/lib/liby.a
	$(RANLIB) $(DESTDIR)/usr/lib/liby.a

clean clobber:
	rm -f *.o liby.a core
