/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef	lint
static	char	rcsid[] = "$Header: init_sysent.c 2.27 1991/07/15 18:29:53 $";
#endif

/*
 * init_sysent.c
 *	System call switch table.
 */

/* $Log: init_sysent.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"

int	nosys();

/* 1.1 processes and protection */
int	sethostid(),gethostid(),sethostname(),gethostname(),getpid();
int	getdomainname(), setdomainname();
int	fork(),rexit(),execv(),execve(),wait();
int	getuid(),setreuid(),getgid(),getgroups(),setregid(),setgroups();
int	getpgrp(),setpgrp();

/* 1.2 memory management */
int	getpagesize();

/* 1.3 signals */
int	sigvec(),sigblock(),sigsetmask(),sigpause(),sigstack();
int	kill(), killpg();
#ifdef	i386
int	sigcleanup(),sigreturn();
#endif	i386

/* 1.4 timing and statistics */
int	gettimeofday(),settimeofday();
int	adjtime();
int	getitimer(),setitimer();

/* 1.5 descriptors */
int	getdtablesize(),dup(),dup2(),close();
int	select(),fcntl(),flock();

/* 1.6 resource controls */
int	getpriority(),setpriority(),getrusage(),getrlimit(),setrlimit();
#ifdef QUOTA
int	quotactl();
#endif QUOTA

/* 1.7 system operation support */
int	mount(), unmount();
int	swapon();
int	sync(),reboot(),sysacct();

/* 2.1 generic operations */
int	read(),write(),readv(),writev(),ioctl();

/* 2.2 file system */
int	chdir(),chroot();
int	mkdir(),rmdir();
int	creat(),open(),mknod(),unlink(),stat(),fstat(),lstat();
int	chown(),fchown(),chmod(),fchmod(),utimes();
int	link(),symlink(),readlink(),rename();
int	lseek(),truncate(),ftruncate(),saccess(),fsync();
int	getdirentries();
int	statfs(), fstatfs();

/* 2.3 communications */
int	socket(),bind(),listen(),accept(),connect();
int	socketpair(),sendto(),send(),recvfrom(),recv();
int	sendmsg(),recvmsg(),shutdown(),setsockopt(),getsockopt();
int	getsockname(),getpeername(),pipe();

int	umask();

/* 2.4 processes */
int	ptrace();

/* 2.5 terminals */

/* 2.6 Sequent */
int	affinity();
int	vm_ctl();
int	tmp_ctl();
int	mmap();
int	munmap();
int	proc_ctl();
int	setdtablesize();
int	db_support();
int	get_vers();
int	adjrst();

/* 2.7 Customer */
int	cust0();
int	cust1();
int	cust2();
int	cust3();
int	cust4();
int	cust5();
int	cust6();
int	cust7();
int	cust8();
int	cust9();

/* nfs */
int	async_daemon();		/* client async daemon */
int	nfs_svc();		/* run nfs server */
int	nfs_getfh();		/* get file handle */

/* BEGIN JUNK */
#define	compat(n, name)	0, nosys
int	profil();		/* 'cuz sys calls are interruptible */
int	vhangup();		/* should just do in exit() */
int	vfork();		/* awaiting fork w/ copy on write */
int	obreak();		/* awaiting new sbrk */
int	readclink();		/* read conditional symbolic link */
int	csymlink();		/* create conditional symblic link */
int	universe();		/* change our universe variable */
int	att_read();		/* att style read */
int	att_fstat();		/* att style fstat */
int 	att_stat();		/* att style stat */
int	msgsys();		/* att messages */
int	semsys();		/* att semaphores */
int	att_creat();		/* att style creat */
int	att_open();		/* att style open */
int	att_mknod();		/* att style mknod */
int	att_chown();		/* att style chown */
int	att_mount();		/* att style mount */
int	att_umount();		/* att style umount */
int	ustat();		/* att ustat */
int	att_setuid();		/* att style setuid */
int	att_unlink();		/* att style unlink */
int	att_acct();		/* att style acct */
/* END JUNK */

struct sysent sysent[] = {
	0, nosys,			/*   0 = indir */
	1, rexit,			/*   1 = exit */
	0, fork,			/*   2 = fork */
	3, read,			/*   3 = read */
	3, write,			/*   4 = write */
	3, open,			/*   5 = open */
	1, close,			/*   6 = close */
	2, creat,			/*   7 = creat */
	2, link,			/*   8 = link */
	1, unlink,			/*   9 = unlink */
	2, execv,			/*  10 = execv */
	1, chdir,			/*  11 = chdir */
	3, mknod,			/*  12 = mknod */
	2, chmod,			/*  13 = chmod */
	3, chown,			/*  14 = chown; now 3 args */
	1, obreak,			/*  15 = old break */
	3, lseek,			/*  16 = lseek */
	0, getpid,			/*  17 = getpid */
	3, att_mount,			/*  18 = system 5 mount */
	1, att_umount,			/*  19 = system 5 umount */
	0, getuid,			/*  20 = getuid */
	4, ptrace,			/*  21 = ptrace */
	2, saccess,			/*  22 = access */
	0, sync,			/*  23 = sync */
	2, kill,			/*  24 = kill */
	2, stat,			/*  25 = stat */
	2, lstat,			/*  26 = lstat */
	1, dup,				/*  27 = dup */
	0, pipe,			/*  28 = pipe */
	4, profil,			/*  29 = profil */
	0, getgid,			/*  30 = getgid */
	1, sysacct,			/*  31 = turn acct off/on */
	3, ioctl,			/*  32 = ioctl */
	1, reboot,			/*  33 = reboot */
	2, symlink,			/*  34 = symlink */
	3, readlink,			/*  35 = readlink */
	3, execve,			/*  36 = execve */
	1, umask,			/*  37 = umask */
	1, chroot,			/*  38 = chroot */
	2, fstat,			/*  39 = fstat */
	0, getpagesize,			/*  40 = getpagesize */
	0, vfork,			/*  41 = vfork */
	0, vhangup,			/*  42 = vhangup */
	2, getgroups,			/*  43 = getgroups */
	2, setgroups,			/*  44 = setgroups */
	1, getpgrp,			/*  45 = getpgrp */
	2, setpgrp,			/*  46 = setpgrp */
	3, setitimer,			/*  47 = setitimer */
	0, wait,			/*  48 = wait */
	1, swapon,			/*  49 = swapon */
	2, getitimer,			/*  50 = getitimer */
	2, gethostname,			/*  51 = gethostname */
	2, sethostname,			/*  52 = sethostname */
	0, getdtablesize,		/*  53 = getdtablesize */
	2, dup2,			/*  54 = dup2 */
	3, fcntl,			/*  55 = fcntl */
	5, select,			/*  56 = select */
	1, fsync,			/*  57 = fsync */
	3, setpriority,			/*  58 = setpriority */
	3, socket,			/*  59 = socket */
	3, connect,			/*  60 = connect */
	3, accept,			/*  61 = accept */
	2, getpriority,			/*  62 = getpriority */
	0, nosys,			/*  63 = used internally */
	4, send,			/*  64 = send */
	4, recv,			/*  65 = recv */
	3, bind,			/*  66 = bind */
	5, setsockopt,			/*  67 = setsockopt */
	2, listen,			/*  68 = listen */
	4, sigvec,			/*  69 = sigvec */
	1, sigblock,			/*  70 = sigblock */
	1, sigsetmask,			/*  71 = sigsetmask */
	1, sigpause,			/*  72 = sigpause */
	2, sigstack,			/*  73 = sigstack */
	3, recvmsg,			/*  74 = recvmsg */
	3, sendmsg,			/*  75 = sendmsg */
	2, gettimeofday,		/*  76 = gettimeofday */
	2, getrusage,			/*  77 = getrusage */
	5, getsockopt,			/*  78 = getsockopt */
	3, readv,			/*  79 = readv */
	3, writev,			/*  80 = writev */
	2, settimeofday,		/*  81 = settimeofday */
	3, fchown,			/*  82 = fchown */
	2, fchmod,			/*  83 = fchmod */
	6, recvfrom,			/*  84 = recvfrom */
	2, setreuid,			/*  85 = setreuid */
	2, setregid,			/*  86 = setregid */
	2, rename,			/*  87 = rename */
	2, truncate,			/*  88 = truncate */
	2, ftruncate,			/*  89 = ftruncate */
	2, flock,			/*  90 = flock */
	6, sendto,			/*  91 = sendto */
	2, shutdown,			/*  92 = shutdown */
	5, socketpair,			/*  93 = socketpair */
	2, mkdir,			/*  94 = mkdir */
	1, rmdir,			/*  95 = rmdir */
	2, utimes,			/*  96 = utimes */
#ifdef	ns32000
	0, nosys,			/*  97 = used internally (sigcleanup) */
#else
	0, sigcleanup,			/*  97 = sigcleanup */
#endif	ns32000
	3, getpeername,			/*  98 = getpeername */
	0, gethostid,			/*  99 = gethostid */
	1, sethostid,			/* 100 = sethostid */
	2, getrlimit,			/* 101 = getrlimit */
	2, setrlimit,			/* 102 = setrlimit */
	2, killpg,			/* 103 = killpg */
	3, getsockname,			/* 104 = getsockname */
	0, cust0,			/* 105 = Reserved for Customer use */
	1, cust1,			/* 106 = Reserved for Customer use */
	2, cust2,			/* 107 = Reserved for Customer use */
	3, cust3,			/* 108 = Reserved for Customer use */
	4, cust4,			/* 109 = Reserved for Customer use */
	5, cust5,			/* 110 = Reserved for Customer use */
	6, cust6,			/* 111 = Reserved for Customer use */
	1, cust7,			/* 112 = Reserved for Customer use */
	1, cust8,			/* 113 = Reserved for Customer use */
	1, cust9,			/* 114 = Reserved for Customer use */
	2, vm_ctl,			/* 115 = VM information/tuning */
	1, affinity,			/* 116 = processor/process affinity */
	3, tmp_ctl,			/* 117 = TMP information and control */
	6, mmap,			/* 118 = mmap */
	2, munmap,			/* 119 = munmap */
	3, proc_ctl,			/* 120 = process control functions */
	1, setdtablesize,		/* 121 = set descriptor table size */
	5, db_support,			/* 122 = interim enhancements for database */
	3, get_vers,			/* 123 = return current OS version */
	0, nosys,			/* 124 = Reserved for Sequent use */
	0, nosys,			/* 125 = Reserved for Sequent use */
	0, nosys,			/* 126 = Reserved for Sequent use */
	0, nosys,			/* 127 = Reserved for Sequent use */
	0, nosys,			/* 128 = Reserved for Sequent use */
	0, nosys,			/* 129 = Reserved for Sequent use */
	3, csymlink,			/* 130 = proto condtional symlink */
	4, readclink,			/* 131 = proto read cond symlink */
	1, universe,			/* 132 = change our universe flag */
	3, att_read,			/* 133 = read, att style */
	2, att_fstat,			/* 134 = fstat, att style */
	2, att_stat,			/* 135 = stat */
	6, msgsys,			/* 136 = msgsys */
	6, semsys,			/* 137 = semsys */
 	2, att_creat,			/* 138 = att style creat */
	3, att_open,			/* 139 = att style open */
	3, att_mknod,			/* 140 = att style mknod */
	3, att_chown,			/* 141 = chown; now 3 args */
	2, ustat,			/* 142 = att ustat */
	1, att_setuid,			/* 143 = att style setuid */
	1, att_unlink,			/* 144 = att style unlink */
	1, att_acct,			/* 145 = att style acct */
	0, nosys,			/* 146 = Reserved for Sequent use */
	0, nosys,			/* 147 = Reserved for Sequent use */
	0, nosys,			/* 148 = Reserved for Sequent use */
	0, nosys,			/* 149 = Reserved for Sequent use */
	4, mount,			/* 150 = mount */
	1, unmount,			/* 151 = unmount */
	2, adjtime,			/* 152 = adjtime */
	4, getdirentries,		/* 153 = getdirentries */
	2, statfs,			/* 154 = statfs */
	2, fstatfs,			/* 155 = fstatfs */
	2, getdomainname,		/* 156 = getdomainname */
	2, setdomainname,		/* 157 = setdomainname */
	1, nfs_svc,			/* 158 = nfs_svc */
	0, async_daemon,		/* 159 = async_daemon */
	2, nfs_getfh,			/* 160 = get file handle */
#ifdef QUOTA
        4, quotactl,                    /* 161 = quotactl */
#else
        0, nosys,                      /* 161 = not configured */
#endif QUOTA
#ifdef	ns32000
	0, nosys,			/* 162 = used internally (sigreturn) */
#else
	1, sigreturn,			/* 162 = sigreturn resevered for sequent use */
#endif ns32000
        2, adjrst,                      /* 163 = call to adjust resident set */
};
int	nsysent = sizeof (sysent) / sizeof (sysent[0]);
