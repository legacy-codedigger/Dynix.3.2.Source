DESTDIR	= $(ATT_DESTDIR)
HU	= a.out.h ar.h assert.h ctype.h curses.h dial.h dir.h errno.h fatal.h\
	  fcntl.h ftw.h grp.h macros.h malloc.h math.h memory.h mnttab.h mon.h\
	  nan.h nlist.h pwd.h regexp.h search.h setjmp.h sgtty.h signal.h\
	  stdio.h string.h term.h termio.h time.h unctrl.h unistd.h ustat.h\
	  utmp.h values.h varargs.h

HS	= sys/_ioctl.h sys/acct.h sys/buf.h sys/crtctl.h sys/dir.h sys/errno.h\
	  sys/ioctl.h sys/param.h sys/shm.h sys/signal.h sys/stat.h\
	  sys/stermio.h sys/sysmacros.h sys/termio.h sys/times.h\
	  sys/ttold.h sys/tty.h sys/types.h sys/universe.h sys/utsname.h\
	  sys/ipc.h sys/msg.h sys/sem.h sys/lock.h sys/mount.h

CP	= sys/ipc.h sys/msg.h sys/sem.h sys/errno.h a.out.h nlist.h sys/stat.h\
	  sys/universe.h sys/mount.h sys/acct.h 

MP	= machine/exec.h machine/param.h machine/vmparam.h machine/machtypes.h

all clobber clean:; rm -f $(CP)

install.headers:& $(CP)
	install -c -m 444 $(HU) $(DESTDIR)/usr/include
	sh -c 'if [ ! -d ../sys/$(MACHINE) ]; then :; else \
		install -c -m 444 $(HS) $(DESTDIR)/usr/include && \
		cd ../sys/$(MACHINE) && \
		  install -c -m 444 $(MP) $(DESTDIR)/usr/include; \
	fi'
	
nlist.h: ../include/nlist.h; rm -f $@; cp $? $@
a.out.h: ../include/a.out.h; rm -f $@; cp $? $@

sys/errno.h \
sys/ipc.h \
sys/msg.h \
sys/sem.h \
sys/mount.h \
sys/universe.h:
	sh -c 'if [ ! -d ../sys/$(MACHINE) ]; then :; else \
		rm -f $@ ; install -c -m 444 ../sys/$(MACHINE)/h/`basename $@` $@; \
	fi'

sys/acct.h:
	sh -c 'if [ ! -d ../sys/$(MACHINE) ]; then :; else \
		rm -f $@; sed -e "s/u_short/ushort/" ../sys/$(MACHINE)/h/acct.h > $@; \
	fi'

sys/stat.h:
	sh -c 'if [ ! -d ../sys/$(MACHINE) ]; then :; else \
		rm -f $@ ; egrep -v "S_IFLNK|S_IFSOCK" ../sys/$(MACHINE)/h/stat.h > $@; \
	fi'
