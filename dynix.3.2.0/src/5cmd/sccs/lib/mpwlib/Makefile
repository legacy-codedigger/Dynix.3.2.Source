
CFLAGS	= $(ATT_INCLUDE) -O
LIB	= ../mpwlib.a
FILES	= fmalloc.o xcreat.o xmsg.o

all:	$(LIB)

$(LIB):& $(FILES)
	@echo "	building $@"
	@$(AR) cru $(LIB) $(FILES)
	@$(RANLIB) $(LIB)

xcreat.o: ../../hdr/defines.h
xmsg.o:	../../hdr/defines.h

install:

clean clobber:
	rm -f *.o core
