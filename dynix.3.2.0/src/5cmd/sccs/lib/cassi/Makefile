
CFLAGS	= $(ATT_INCLUDE) -O
LIB	= ../cassi.a
FILES	= gf.o cmrcheck.o deltack.o error.o filehand.o

all:	$(LIB)

$(LIB):& $(FILES)
	@echo "	building $@"
	@$(AR) cru $(LIB) $(FILES)
	@$(RANLIB) $(LIB)

gf.o:		gf.c ../../hdr/filehand.h
cmrcheck.o:	cmrcheck.c ../../hdr/filehand.h
deltack.o:	deltack.c ../../hdr/filehand.h ../../hdr/had.h ../../hdr/defines.h
filehand.o:	filehand.c ../../hdr/filehand.h

install:

clean clobber:
	rm -f *.o core
