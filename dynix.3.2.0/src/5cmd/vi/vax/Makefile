# sccsid = "@(#)makefile.usg	1.21"
#
# Ex skeletal makefile for USG Unix (5.0, System V, etc) on a VAX or 3B.
#
# NB: This makefile doesn't indicate any dependencies on header files.
#
# Ex is very large - this version will not fit on PDP-11's without overlay
# software.  Things that can be turned off to save
# space include LISPCODE (-l flag, showmatch and lisp options), UCVISUAL
# (visual \ nonsense on upper case only terminals), CHDIR (the undocumented
# chdir command.)  CRYPT includes the code to edit encrypted files (the -x
# option, like ed.)  VMUNIX makes ex considerably larger, raising many limits
# and improving speed and simplicity of maintenance.  It is suitable only
# for a VAX or other large machine, and then probably only in a paged system.
#
# Don't define VFORK unless your system has the VFORK system call,
# which is like fork but the two processes have only one data space until the
# child execs. This speeds up ex by saving the memory copy.
#
# If your system expands tabs to 4 spaces you should -DTABS=4 below
#
DESTDIR	= $(ATT_DESTDIR)
CC	= $(ATT_CC)
INCLUDE	= $(ATT_INCLUDE)
BINDIR	= /usr/bin
NBINDIR	= /usr/lbin
LIBDIR	= /usr/lib
FOLD	= ../misc/fold
CTAGS	= ../misc/ctags
XSTR	= ../misc/xstr
DEBUGFLAGS=	-DTRACE -g
NONDEBUGFLAGS=	-O
DEB=	${NONDEBUGFLAGS} # or ${DEBUGFLAGS} to to debug
OPTIONS	= -DUSG -DSTDIO -DVMUNIX
CFLAGS	=	$(INCLUDE) -DTABS=8 -DSINGLE ${OPTIONS} ${DEB}
LDFLAGS	=
TERMLIB	=	-lcurses
MKSTR=	../misc/mkstr
CXREF=	../misc/cxref
EX	= ex
PR=	pr
OBJS=	ex.O ex_addr.O ex_cmds.O ex_cmds2.O ex_cmdsub.O \
	ex_data.O ex_extern.O ex_get.O ex_io.O ex_put.O ex_re.O \
	ex_set.O ex_subr.O ex_temp.O ex_tty.O ex_unix.O \
	ex_v.O ex_vadj.O ex_vget.O ex_vmain.O ex_voper.O \
	ex_vops.O ex_vops2.O ex_vops3.O ex_vput.O ex_vwind.O \
	printf.O bcopy.O # strings.O
XOBJS=	ex.x ex_addr.x ex_cmds.x ex_cmds2.x ex_cmdsub.x \
	ex_data.x ex_extern.x ex_get.x ex_io.x ex_put.x ex_re.x \
	ex_set.x ex_subr.x ex_temp.x ex_tty.x ex_unix.x \
	ex_v.x ex_vadj.x ex_vget.x ex_vmain.x ex_voper.x \
	ex_vops.x ex_vops2.x ex_vops3.x ex_vput.x ex_vwind.x \
	printf.x bcopy.x # strings.x
EHDRS=	ex.h ex_argv.h ex_re.h ex_temp.h ex_tty.h ex_tune.h ex_vars.h ex_vis.h
SRC1=	ex.c ex_addr.c ex_cmds.c ex_cmds2.c ex_cmdsub.c
SRC2=	ex_data.c ex_extern.c ex_get.c ex_io.c ex_put.c ex_re.c
SRC3=	ex_set.c ex_subr.c ex_temp.c ex_tty.c ex_unix.c
SRC4=	ex_v.c ex_vadj.c ex_vget.c ex_vmain.c ex_voper.c
SRC5=	ex_vops.c ex_vops2.c ex_vops3.c ex_vput.c ex_vwind.c
SRC6=	printf.c bcopy.c expreserve.c exrecover.c
MISC=	makefile READ_ME rofix
I	= install -s $? $@
IL	= rm -f $@; ln $? $@
IC	= install -c -m 644 $? $@
IR	= install -c -s -m 4755 -o root $? $@
DB	= $(DESTDIR)/$(BINDIR)
CDB	= $(ATT_CRYPT_DESTDIR)/$(BINDIR)
DL	= $(DESTDIR)/$(LIBDIR)
CDL	= $(ATT_CRYPT_DESTDIR)/$(LIBDIR)

IFILES	=\
	$(DB)/vi\
	$(DB)/view\
	$(DB)/edit\
	$(DB)/vedit\
	$(DL)/exrecover\
	$(DL)/expreserve\
	$(CDB)/vi\
	$(CDB)/view\
	$(CDB)/edit\
	$(CDB)/vedit

.SUFFIXES: .c .o .x .O

.c.O:
	$(CC) $(CFLAGS) -UCRYPT -c $*.c; mv $*.o $*.O

.c.x:
	$(CC) $(CFLAGS) -DCRYPT -c $*.c; mv $*.o $*.x

all:	$(EX) ex_crypt exrecover expreserve

$(EX):&  $(OBJS)
	${CC} ${LDFLAGS} ${OBJS} ${TERMLIB} -o ${EX}

pex_crypt:&  $(XOBJS)
	${ATT_CRYPT_CC} ${LDFLAGS} ${XOBJS} ${TERMLIB} -lcrypt -o ex_crypt

ex_crypt:
	sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		make $(MFLAGS) $(MRULES) pex_crypt; \
	fi'

${OBJS}: ex_vars.h

${XOBJS}: ex_vars.h

bcopy.o:	bcopy.c
	${CC} -c ${CFLAGS} bcopy.c

exrecover: exrecover.o ex_extern.o
	${CC} ${CFLAGS} exrecover.o ex_extern.o -o exrecover

exrecover.o: exrecover.c
	${CC} ${CFLAGS} -c -O exrecover.c

expreserve: expreserve.o
	${CC} expreserve.o -o expreserve

expreserve.o:
	${CC} ${CFLAGS} -c -O expreserve.c

clean:
#	If we dont have ex we cant make it so dont rm ex_vars.h
	-rm -f exstrings strings core errs trace *.o x*.[cs] \
		mkstrings mkxstrings expreserve exrecover *.x *.O

clobber:
	-rm -f $(EX) exrecover expreserve ex_crypt

# install in standard place (/usr/bin)
install: $(DB)/ex $(CDB)/ex pinstall
pinstall:& $(IFILES)

$(DB)/ex: $(EX); $(I)
$(DB)/vi: $(DB)/ex; $(IL)
$(DB)/view: $(DB)/ex; $(IL)
$(DB)/edit: $(DB)/ex; $(IL)
$(DB)/vedit: $(DB)/ex; $(IL)

$(CDB)/ex: ex_crypt; -if [ "$(EXPORT)" != TRUE ]; then $(I); fi
$(CDB)/vi: $(CDB)/ex; -if [ "$(EXPORT)" != TRUE ]; then $(IL); fi
$(CDB)/view: $(CDB)/ex; -if [ "$(EXPORT)" != TRUE ]; then $(IL); fi
$(CDB)/edit: $(CDB)/ex; -if [ "$(EXPORT)" != TRUE ]; then $(IL); fi
$(CDB)/vedit: $(CDB)/ex; -if [ "$(EXPORT)" != TRUE ]; then $(IL); fi

$(DL)/exrecover: exrecover; $(IR)
$(DL)/expreserve: expreserve; $(IR)
