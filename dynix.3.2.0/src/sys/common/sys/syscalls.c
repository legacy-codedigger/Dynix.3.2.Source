/* $Copyright:	$
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

/*
 * $Header: syscalls.c 2.15 91/03/05 $
 *
 * syscalls.c
 *	System call names.
 */

/* $Log:	syscalls.c,v $
 */

char *syscallnames[] = {
	"indir",		/*   0 = indir */
	"exit",			/*   1 = exit */
	"fork",			/*   2 = fork */
	"read",			/*   3 = read */
	"write",		/*   4 = write */
	"open",			/*   5 = open */
	"close",		/*   6 = close */
	"creat",		/*   7 = creat */
	"link",			/*   8 = link */
	"unlink",		/*   9 = unlink */
	"execv",		/*  10 = execv */
	"chdir",		/*  11 = chdir */
	"mknod",		/*  12 = mknod */
	"chmod",		/*  13 = chmod */
	"chown",		/*  14 = chown; now 3 args */
	"break",		/*  15 = break */
	"lseek",		/*  16 = lseek */
	"getpid",		/*  17 = getpid */
	"att_mount",		/*  18 = system 5 mount */
	"att_umount",		/*  19 = system 5 umount */
	"getuid",		/*  20 = getuid */
	"ptrace",		/*  21 = ptrace */
	"access",		/*  22 = access */
	"sync",			/*  23 = sync */
	"kill",			/*  24 = kill */
	"stat",			/*  25 = stat */
	"lstat",		/*  26 = lstat */
	"dup",			/*  27 = dup */
	"pipe",			/*  28 = pipe */
	"profil",		/*  29 = profil */
	"getgid",		/*  30 = getgid */
	"sysacct",		/*  31 = turn acct off/on */
	"ioctl",		/*  32 = ioctl */
	"reboot",		/*  33 = reboot */
	"symlink",		/*  34 = symlink */
	"readlink",		/*  35 = readlink */
	"execve",		/*  36 = execve */
	"umask",		/*  37 = umask */
	"chroot",		/*  38 = chroot */
	"fstat",		/*  39 = fstat */
	"getpagesize",		/*  40 = getpagesize */
	"vfork",		/*  41 = vfork */
	"vhangup",		/*  42 = vhangup */
	"getgroups",		/*  43 = getgroups */
	"setgroups",		/*  44 = setgroups */
	"getpgrp",		/*  45 = getpgrp */
	"setpgrp",		/*  46 = setpgrp */
	"setitimer",		/*  47 = setitimer */
	"wait",			/*  48 = wait */
	"swapon",		/*  49 = swapon */
	"getitimer",		/*  50 = getitimer */
	"gethostname",		/*  51 = gethostname */
	"sethostname",		/*  52 = sethostname */
	"getdtablesize",	/*  53 = getdtablesize */
	"dup2",			/*  54 = dup2 */
	"fcntl",		/*  55 = fcntl */
	"select",		/*  56 = select */
	"fsync",		/*  57 = fsync */
	"setpriority",		/*  58 = setpriority */
	"socket",		/*  59 = socket */
	"connect",		/*  60 = connect */
	"accept",		/*  61 = accept */
	"getpriority",		/*  62 = getpriority */
	"bad syscall",		/*  63 = used internally */
	"send",			/*  64 = send */
	"recv",			/*  65 = recv */
	"bind",			/*  66 = bind */
	"setsockopt",		/*  67 = setsockopt */
	"listen",		/*  68 = listen */
	"sigvec",		/*  69 = sigvec */
	"sigblock",		/*  70 = sigblock */
	"sigsetmask",		/*  71 = sigsetmask */
	"sigpause",		/*  72 = sigpause */
	"sigstack",		/*  73 = sigstack */
	"recvmsg",		/*  74 = recvmsg */
	"sendmsg",		/*  75 = sendmsg */
	"gettimeofday",		/*  76 = gettimeofday */
	"getrusage",		/*  77 = getrusage */
	"getsockopt",		/*  78 = getsockopt */
	"readv",		/*  79 = readv */
	"writev",		/*  80 = writev */
	"settimeofday",		/*  81 = settimeofday */
	"fchown",		/*  82 = fchown */
	"fchmod",		/*  83 = fchmod */
	"recvfrom",		/*  84 = recvfrom */
	"setreuid",		/*  85 = setreuid */
	"setregid",		/*  86 = setregid */
	"rename",		/*  87 = rename */
	"truncate",		/*  88 = truncate */
	"ftruncate",		/*  89 = ftruncate */
	"flock",		/*  90 = flock */
	"sendto",		/*  91 = sendto */
	"shutdown",		/*  92 = shutdown */
	"socketpair",		/*  93 = socketpair */
	"mkdir",		/*  94 = mkdir */
	"rmdir",		/*  95 = rmdir */
	"utimes",		/*  96 = utimes */
	"sigcleanup",		/*  97 = sigcleanup */
	"getpeername",		/*  98 = getpeername */
	"gethostid",		/*  99 = gethostid */
	"sethostid",		/* 100 = sethostid */
	"getrlimit",		/* 101 = getrlimit */
	"setrlimit",		/* 102 = setrlimit */
	"killpg",		/* 103 = killpg */
	"getsockname",		/* 104 = getsockname */
	"cust0",		/* 105 = Reserved for Customer use */
	"cust1",		/* 106 = Reserved for Customer use */
	"cust2",		/* 107 = Reserved for Customer use */
	"cust3",		/* 108 = Reserved for Customer use */
	"cust4",		/* 109 = Reserved for Customer use */
	"cust5",		/* 110 = Reserved for Customer use */
	"cust6",		/* 111 = Reserved for Customer use */
	"cust7",		/* 112 = Reserved for Customer use */
	"cust8",		/* 113 = Reserved for Customer use */
	"cust9",		/* 114 = Reserved for Customer use */
	"vm_ctl",		/* 115 = vm_ctl	*/
	"affinity",		/* 116 = processor/process affinity */
	"tmp_ctl",		/* 117 = tmp_ctl */
	"mmap",			/* 118 = mmap */
	"munmap",		/* 119 = munmap */
	"proc_ctl",		/* 120 = process control functions */
	"setdtablesize",	/* 121 = set descriptor table size */
	"db_support",		/* 122 = interim enhancements for database */
	"get_vers",		/* 123 = Reserved for Sequent use */
	"#124",			/* 124 = Reserved for Sequent use */
	"#125",			/* 125 = Reserved for Sequent use */
	"#126",			/* 126 = Reserved for Sequent use */
	"#127",			/* 127 = Reserved for Sequent use */
	"#128",			/* 128 = Reserved for Sequent use */
	"#129",			/* 129 = Reserved for Sequent use */
	"csymlink",		/* 130 = proto condtional symlink */
	"readclink",		/* 131 = proto read cond symlink */
	"universe",		/* 132 = change our universe flag */
	"att_read",		/* 133 = read, att style */
	"att_fstat",		/* 134 = fstat, att style */
	"att_stat",		/* 135 = stat, att style */
	"msgsys",		/* 136 = msgsys */
	"semsys",		/* 137 = semsys */
 	"att_creat",		/* 138 = att style creat */
	"att_open",		/* 139 = att style open */
	"att_mknod",		/* 140 = att style mknod */
	"att_chown",		/* 141 = chown; now 3 args */
	"ustat",		/* 142 = att ustat */
	"att_setuid",		/* 143 = att style setuid */
	"att_unlink",		/* 144 = att style unlink */
	"att_acct",		/* 145 = att style acct */
	"#146",			/* 146 = Reserved for Sequent use */
	"#147",			/* 147 = Reserved for Sequent use */
	"#148",			/* 148 = Reserved for Sequent use */
	"#149",			/* 149 = Reserved for Sequent use */
	"mount",		/* 150 = mount */
	"unmount",		/* 151 = nfs_mount */
	"adjtime",		/* 152 = adjtime */
	"getdirentries",	/* 153 = getdirentries */
	"statfs",		/* 154 = statfs */
	"fstatfs",		/* 155 = fstatfs */
	"getdomainname",	/* 156 = getdomainname */
	"setdomainname",	/* 157 = setdomainname */
	"nfs_svc",		/* 158 = nfs_svc */
	"async_daemon",		/* 159 = async_daemon */
	"nfs_getfh",		/* 160 = get file handle */
#ifdef QUOTA
        "quotactl",             /* 161 = quotactl */
#else
        "#161",                 /* 161 = nullsys */
#endif
};
