#	@(#)makefile	1.4
#
#	Curses Library Low Level Makefile
#
#	CFLAGS has -DVIDEO & -DKEYPAD for the libcurses.a library.  They could
#		be put into a separate variable if desired, but I saw no reason
#		for doing so.
#	DFLAGS, TFLAGS, & PFLAGS are for debugging, tracing, & profiling
#		respectively.  They all contain the DEBUG flag because it is
#		thought that any of the styles of compiling is still debugging.
#		Also the DEBUG flag will create very large files and it is not
#		recommended that all of the files be compiled with the DEBUG
#		flag defined.  This will make 'ar' die because the library will
#		exceed 2065 blocks in size.  But I see no reason not to compile
#		individual or groups of files with these flags defined.  These
#		flags can't be used on a pdp11/70 or other small machines
#		because of the huge sizes of .o files produced for the library.
#	FILES is used for the naming of the object files.
#	O is used to destinguish between compiling, debugging, tracing, &
#		profiling.
#	MINICURSES is a flag used to compile a small version of libcurses.
#		It should not be used for the compiling of libcurses.a.  It is
#		strictly for the application programmers convenience.
#		If MINICURSES is desired with ti4 or show then type:
#			"make <[ti4][show] MINICURSES=-DMINICURSES"
#
##
# ROOT		=
##
DESTDIR		= $(ATT_DESTDIR)
INCLUDE		= $(ATT_INCLUDE)
CC		= $(ATT_CC)
LIB		= $(DESTDIR)/usr/lib
CURSES		= libcurses.a
BIN		= $(DESTDIR)/usr/bin
O		= o
MINICURSES	=
CFLAGS		= -DVIDEO -DKEYPAD -O $(INCLUDE)
DFLAGS		= -DVIDEO -DKEYPAD -DDEBUG -I. $(INCLUDE)
TFLAGS		= -DVIDEO -DKEYPAD -DDEBUG -O -I. $(INCLUDE)
PFLAGS		= -DVIDEO -DKEYPAD -DDEBUG -p -O -I. $(INCLUDE)

##
# (BUG FIX) Removed duplicate definitions
#   1.  delay_output() is already exported by "naps.c" (remove delayoutpt.o)
#   2.  gettmode() is already exported by "miniinit.c" (remove gettmode.o)
#   3.  setterm() already exported by "miniinit.c" (remove setterm.o)
BUGS		= delayoutpt.$(O) gettmode.$(O) setterm.$(O)
##

FILES		= __cflush.$(O) __sscans.$(O) _blanks.$(O) _c_clean.$(O) \
		_clearhl.$(O) _clearline.$(O) _comphash.$(O) _delay.$(O) \
		_delchars.$(O) _dellines.$(O) _dumpwin.$(O) _ec_quit.$(O) \
		_fixdelay.$(O) _forcehl.$(O) _hlmode.$(O) _id_char.$(O) \
		_init_cost.$(O) _inschars.$(O) _insmode.$(O) _kpmode.$(O) \
		_line_free.$(O) _ll_move.$(O) _outch.$(O) _outchar.$(O) \
		_pos.$(O) _reset.$(O) _scrdown.$(O) _scrollf.$(O) _sethl.$(O) \
		_setmode.$(O) _setwind.$(O) _shove.$(O) _sprintw.$(O) \
		_sputc.$(O) _syncmodes.$(O) _tscroll.$(O) _window.$(O) \
		addch.$(O) addstr.$(O) baudrate.$(O) beep.$(O) box.$(O) \
		capnames.$(O) cbreak.$(O) chktypeahd.$(O) clear.$(O) \
		clearok.$(O) clreolinln.$(O) clrtobot.$(O) clrtoeol.$(O) \
		cntcostfn.$(O) crmode.$(O) curses.$(O) def_prog.$(O) \
		def_shell.$(O) delch.$(O) deleteln.$(O) \
		delwin.$(O) doprnt.$(O) doscan.$(O) doupdate.$(O) draino.$(O) \
		echo.$(O) endwin.$(O) erase.$(O) erasechar.$(O) fixterm.$(O) \
		flash.$(O) flushinp.$(O) getch.$(O) getstr.$(O) idlok.$(O) \
		has_ic.$(O) has_il.$(O) idln.getst.$(O) \
		initkeypad.$(O) initscr.$(O) insch.$(O) insertln.$(O) \
		intrflush.$(O) keypad.$(O) killchar.$(O) leaveok.$(O) \
		line_alloc.$(O) ll_refresh.$(O) longname.$(O) m_addch.$(O) \
		m_addstr.$(O) m_clear.$(O) m_erase.$(O) m_move.$(O) meta.$(O) \
		m_refresh.$(O) m_tstp.$(O) makenew.$(O) miniinit.$(O) \
		move.$(O) mvcur.$(O) mvprintw.$(O) mvscanw.$(O) mvwin.$(O) \
		mvwprintw.$(O) mvwscanw.$(O) naps.$(O) newpad.$(O) \
		newterm.$(O) newwin.$(O) nl.$(O) nocbreak.$(O) nocrmode.$(O) \
		nodelay.$(O) noecho.$(O) nonl.$(O) noraw.$(O) nttychktrm.$(O) \
		overlay.$(O) overwrite.$(O) pnoutrfrsh.$(O) prefresh.$(O) \
		printw.$(O) putp.$(O) raw.$(O) reset_prog.$(O) resetshell.$(O) \
		resetterm.$(O) resetty.$(O) restarttrm.$(O) saveterm.$(O) \
		savetty.$(O) scanw.$(O) scroll.$(O) scrollok.$(O) select.$(O) \
		set_term.$(O) setbuffred.$(O) setupterm.$(O) \
		showstring.$(O) subwin.$(O) tgetent.$(O) tgetflag.$(O) \
		tgetnum.$(O) tgetstr.$(O) tgoto.$(O) touchwin.$(O) tparm.$(O) \
		tputs.$(O) traceonoff.$(O) tstp.$(O) two.twostr.$(O) \
		typeahead.$(O) unctrl.$(O) vidattr.$(O) vidputs.$(O) \
		vsprintf.$(O) vsscanf.$(O) wattroff.$(O) wattron.$(O) \
		wattrset.$(O) wnoutrfrsh.$(O) wprintw.$(O) wrefresh.$(O) \
		writechars.$(O) wscanw.$(O) wstandend.$(O) wstandout.$(O)

.SUFFIXES:	.o .d .t .p

.c.o:	
			$(CC) $(CFLAGS) $(MINICURSES) -c $*.c

.c.d:	
			if [ -f $*.o ] ; then mv $*.o tmp ; fi
			$(CC) $(DFLAGS) $(MINICURSES) -c $*.c
			mv $*.o $*.d
			if [ -f tmp ] ; then mv tmp $*.o
.c.t:	
			if [ -f $*.o ] ; then mv $*.o tmp ; fi
			$(CC) $(TFLAGS) $(MINICURSES) -c $*.c
			mv $*.o $*.d
			if [ -f tmp ] ; then mv tmp $*.o

.c.p:	
			if [ -f $*.o ] ; then mv $*.o tmp ; fi
			$(CC) $(PFLAGS) $(MINICURSES) -c $*.c
			mv $*.o $*.d
			if [ -f tmp ] ; then mv tmp $*.o

all:		tic $(CURSES)

$(CURSES):	parallel curses.h curshdr.h term.h unctrl.h curses.ext curses.c
		@rm -f tic.x.o tic.o capnames.o capnames.x.o
		@echo "	building $(CURSES)"
		@echo '       Fixing modes, groups, and owners on *.o'
		@-chown root *.$(O) > /dev/null 2>&1
		@-chgrp daemon *.$(O) > /dev/null 2>&1
		@chmod 644 *.$(O)
		@$(AR) rcu $(CURSES) *.$(O)

parallel:&	$(FILES)

debug:
		$(MAKE) O=d CURSES=libdcurses.a

trace:
		$(MAKE) O=t CURSES=libtcurses.a

profile:
		$(MAKE) O=p CURSES=libpcurses.a

show:		libcurses.a show.o
		$(CC) $(CFLAGS) $(MINICURSES) show.o libcurses.a -o show
		rm show.o

tic:		tic.o tic.x.o capnames.o capnames.x.o
		$(CC) $(CFLAGS) $(MINICURSES) tic.o capnames.o -o tic
		-rm -f tic.x
		cc tic.x.o capnames.x.o -o tic.x
		-rm -f tic.x.o tic.o capnames.o capnames.x.o

tic.o tic.x.o:	tic.c
	$(CC) $(CFLAGS) $(MINICURSES) -c $*.c
	rm -f tic.x.c
	ln -s tic.c tic.x.c
	cc -O -c $*.x.c
	rm tic.x.c

capnames.o capnames.x.o: capnames.c
	$(CC) $(CFLAGS) $(MINICURSES) -c $*.c
	rm -f capnames.x.c
	ln -s capnames.c capnames.x.c
	cc -O -c $*.x.c
	rm capnames.x.c

ti4:		libcurses.a ti4.o
		$(CC) $(CFLAGS) $(MINICURSES) ti4$(O) libcurses.a -o ti4
		rm ti4.o

ti4.o:		
		$(CC) mkti4.c -O -o mkti4
		mkti4 >ti4.c
		$(CC) $(CFLAGS) $(MINICURSES) ti4.c -O -c

clobber clean:	
		rm -f tic.x core *.o libcurses.a tic

install:
		install -c -m 644 libcurses.a $(LIB)/libcurses.a
		$(RANLIB) $(LIB)/libcurses.a
		-if [ "$(EXPORT)" != TRUE ]; then \
			install -m 644 libcurses.a $(ATT_CRYPT_DESTDIR)/usr/lib/libcurses.a; \
			$(RANLIB) $(ATT_CRYPT_DESTDIR)/usr/lib/libcurses.a; \
		fi
		install -s tic $(BIN)/tic
