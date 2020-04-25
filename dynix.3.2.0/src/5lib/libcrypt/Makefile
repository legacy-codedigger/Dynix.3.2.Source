
DESTDIR	= $(ATT_DESTDIR)
CFLAGS	= $(ATT_INCLUDE) -O
OBJS	= crypt.o
DES_OBJS = DEScrypt.o

all:	libcrypt.a DESlibcrypt.a

libcrypt.a: $(OBJS)
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 $(OBJS)
	@-chgrp daemon $(OBJS) > /dev/null 2>&1
	@-chown  root $(OBJS) > /dev/null 2>&1
	$(AR) cru libcrypt.a $(OBJS)

DESlibcrypt.a: $(DES_OBJS)
	@echo '	Fixing modes, groups, and owners on *.o'
	@-chgrp daemon $(DES_OBJS) > /dev/null 2>&1
	@-chown  root $(DES_OBJS) > /dev/null 2>&1
	sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		chmod    644 $(DES_OBJS); \
		$(AR) cru DESlibcrypt.a $(DES_OBJS); \
	fi'

crypt.o: crypt.c
	sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		$(CC) $(CFLAGS) -c crypt.c; \
	fi'

crypt.c:

DEScrypt.o: crypt.c
	sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		rm -f DEScrypt.c; \
		ln -s crypt.c DEScrypt.c; \
		$(CC) $(CFLAGS) -DCRYPT=1 -c DEScrypt.c; \
	fi'

install:
	install -m 644 libcrypt.a $(DESTDIR)/usr/lib/libcrypt.a
	$(RANLIB) $(DESTDIR)/usr/lib/libcrypt.a
	sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		install -m 644 DESlibcrypt.a $(ATT_CRYPT_DESTDIR)/usr/lib/libcrypt.a; \
		$(RANLIB) $(ATT_CRYPT_DESTDIR)/usr/lib/libcrypt.a; \
	fi'

clean clobber:
	rm -f libcrypt.a DESlibcrypt.a DEScrypt.[co]
	sh -c 'if [ ! -f crypt.c ]; then :; else \
		rm -f crypt.o; \
	fi'
