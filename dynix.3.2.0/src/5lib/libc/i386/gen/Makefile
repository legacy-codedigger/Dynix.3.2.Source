SRCDIR	= ../../../../lib/libc/$(MACHINE)/gen
INCLUDE	= $(ATT_INCLUDE)
CFLAGS	= -O -I. $(INCLUDE)
OBJS	= exit.o longjmp.o setjmp.o fpvalues.o _ppinit.o _profbrk.o
LIB	= genlib genlib_p

# Reorder suffix list so .s's are found before .c's. 
.SUFFIXES:
.SUFFIXES: .out .o .s .c .e .r .f .y .l .p

.s.o:
	$(CPP) -DPROF $(INCLUDE) $*.s | $(AS) -o $*.o && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CPP) -UPROF $(INCLUDE) $*.s | $(AS) -o $*.o && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o
.c.o:
	$(CC) -p -DPROF $(CFLAGS) -c $*.c && \
	$(LD) -X -r $*.o -o profiled/$*.o && \
	$(CC)    -UPROF $(CFLAGS) -c $*.c && \
	$(LD) -x -r $*.o -o $*.O && mv $*.O $*.o

all		: $(LIB)
$(LIB)		: profiled parallel
		  @echo "	building profiled genlib" && \
		  cd profiled && $(AR) cru ../genlib_p $(OBJS) && \
		  echo "	building normal genlib" && \
		  cd .. && $(AR) cru genlib $(OBJS)

parallel	:& $(OBJS)
$(OBJS)		: DEFS.h
setjmp.o 	: Setjmp.h
longjmp.o	: Setjmp.h
setjmp.s	: $(SRCDIR)/_setjmp.s
		  rm -f $@; sed 's/_setjmp/setjmp/g' < $? > $@
longjmp.s	: $(SRCDIR)/_longjmp.s
		  rm -f $@; sed 's/_longjmp/longjmp/g' < $? > $@
DEFS.h		: $(SRCDIR)/../DEFS.h
		  rm -f $@; ln -s $?
Setjmp.h	: $(SRCDIR)/Setjmp.h
		  rm -f $@; ln -s $?
fpvalues.c	: $(SRCDIR)/fpvalues.c
		  rm -f $@; ln -s $?
_ppinit.c	: $(SRCDIR)/_ppinit.c
		  rm -f $@; ln -s $?
_profbrk.c	: $(SRCDIR)/_profbrk.c
		  rm -f $@; ln -s $?
profiled	:;mkdir profiled
clean clobber	:;rm -rf $(OBJS) $(LIB) profiled setjmp.s longjmp.s \
		  DEFS.h Setjmp.h fpvalues.c _ppinit.c _profbrk.c
