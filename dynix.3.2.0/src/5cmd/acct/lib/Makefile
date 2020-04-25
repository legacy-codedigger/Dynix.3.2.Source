
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CC	= $(ATT_CC)
LIB = a.a
INCDIR = ..
CFLAGS	= $(ATT_INCLUDE) -O -I$(INCDIR)

FILES= pnpsplit.o copyn.o devtolin.o expand.o lintodev.o namtouid.o\
	tmless.o tmsecs.o uidtonam.o substr.o regex.o regcmp.o

all:	$(LIB)

$(LIB):	$(FILES)
	$(ATT_AR) cr $(LIB) $(FILES)
	$(ATT_RANLIB) $(LIB)

clean clobber:
	-rm -f *.o $(LIB)
