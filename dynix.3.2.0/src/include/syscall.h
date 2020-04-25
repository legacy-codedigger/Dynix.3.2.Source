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

/* $Header: syscall.h 2.17 1991/04/16 17:00:01 $ */

/* $Log: syscall.h,v $
 *
 */

#define	SYS_exit	1
#define	SYS_fork	2
#define	SYS_read	3
#define	SYS_write	4
#define	SYS_open	5
#define	SYS_close	6
#define	SYS_creat	7
#define	SYS_link	8
#define	SYS_unlink	9
#define	SYS_execv	10
#define	SYS_chdir	11
#define	SYS_mknod	12
#define	SYS_chmod	13
#define	SYS_chown	14
#define SYS_brk		15
#define	SYS_lseek	16
#define	SYS_getpid	17
#define	SYS_att_mount	18
#define	SYS_umount	19
#define	SYS_getuid	20
#define	SYS_ptrace	21
#define	SYS_access	22
#define	SYS_sync	23
#define	SYS_kill	24
#define	SYS_stat	25
#define	SYS_lstat	26
#define	SYS_dup		27
#define	SYS_pipe	28
#define	SYS_profil	29
#define	SYS_getgid	30
#define	SYS_acct	31
#define	SYS_ioctl	32
#define	SYS_reboot	33
#define	SYS_symlink	34
#define	SYS_readlink	35
#define	SYS_execve	36
#define	SYS_umask	37
#define	SYS_chroot	38
#define	SYS_fstat	39
#define	SYS_getpagesize 40
#define	SYS_vfork	41
#define	SYS_vhangup	42
#define	SYS_getgroups	43
#define	SYS_setgroups	44
#define	SYS_getpgrp	45
#define	SYS_setpgrp	46
#define	SYS_setitimer	47
#define	SYS_wait	48
#define	SYS_swapon	49
#define	SYS_getitimer	50
#define	SYS_gethostname	51
#define	SYS_sethostname	52
#define	SYS_getdtablesize 53
#define	SYS_dup2	54
#define	SYS_fcntl	55
#define	SYS_select	56
#define	SYS_fsync	57
#define	SYS_setpriority	58
#define	SYS_socket	59
#define	SYS_connect	60
#define	SYS_accept	61
#define	SYS_getpriority	62
				/* 63 is used internally */
#define	SYS_send	64
#define	SYS_recv	65
#define	SYS_bind	66
#define	SYS_setsockopt	67
#define	SYS_listen	68
#define	SYS_sigvec	69
#define	SYS_sigblock	70
#define	SYS_sigsetmask	71
#define	SYS_sigpause	72
#define	SYS_sigstack	73
#define	SYS_recvmsg	74
#define	SYS_sendmsg	75
#define	SYS_gettimeofday 76
#define	SYS_getrusage	77
#define	SYS_getsockopt	78
#define	SYS_readv	79
#define	SYS_writev	80
#define	SYS_settimeofday 81
#define	SYS_fchown	82
#define	SYS_fchmod	83
#define	SYS_recvfrom	84
#define	SYS_setreuid	85
#define	SYS_setregid	86
#define	SYS_rename	87
#define	SYS_truncate	88
#define	SYS_ftruncate	89
#define	SYS_flock	90
#define	SYS_sendto	91
#define	SYS_shutdown	92
#define	SYS_socketpair	93
#define	SYS_mkdir	94
#define	SYS_rmdir	95
#define	SYS_utimes	96
#define	SYS_sigcleanup	97	/* 97 is used with signals only - replaced
				 * in v3.1 by 162 sigreturn
				 */
#define	SYS_getpeername	98
#define	SYS_gethostid	99
#define	SYS_sethostid	100
#define	SYS_getrlimit	101
#define	SYS_setrlimit	102
#define	SYS_killpg	103
#define	SYS_getsockname	104
#define	SYS_cust0	105	/* 105 = Reserved for Customer use */
#define	SYS_cust1	106	/* 106 = Reserved for Customer use */
#define	SYS_cust2	107	/* 107 = Reserved for Customer use */
#define	SYS_cust3	108	/* 108 = Reserved for Customer use */
#define	SYS_cust4	109	/* 109 = Reserved for Customer use */
#define	SYS_cust5	110	/* 110 = Reserved for Customer use */
#define	SYS_cust6	111	/* 111 = Reserved for Customer use */
#define	SYS_cust7	112	/* 112 = Reserved for Customer use */
#define	SYS_cust8	113	/* 113 = Reserved for Customer use */
#define	SYS_cust9	114	/* 114 = Reserved for Customer use */

#define	SYS_vm_ctl	115	/* 115 = start of SEQUENT added syscalls */
#define	SYS_tmp_affinity 116
#define	SYS_tmp_ctl	117
#define	SYS_mmap	118
#define	SYS_munmap	119
#define	SYS_proc_ctl	120
#define	SYS_setdtablesize 121
#define SYS_dbsupport	122
#define SYS_get_vers	123

#define	SYS_csymlink	130	/* 130 = start of System V support */
#define	SYS_readclink	131
#define	SYS_universe	132
#define	SYSV_read	133
#define	SYSV_fstat	134
#define	SYSV_stat	135
#define	SYS_msgsys	136
#define	SYS_semsys	137
#define	SYSV_creat	138
#define	SYSV_open	139
#define	SYSV_mknod	140
#define	SYSV_chown	141	/* Sys V mount/umount are 18/19 */
#define SYS_ustat	142
#define	SYS_att_setuid	143
#define	SYSV_unlink	144
#define	SYSV_acct	145

#define	SYS_mount	150	/* base VFS/NFS support */
#define	SYS_unmount	151
#define	SYS_adjtime	152
#define	SYS_getdirentries 153
#define	SYS_statfs	154
#define	SYS_fstatfs	155
#define	SYS_getdomainname 156
#define	SYS_setdomainname 157

#define	SYS_nfssvc	158	/* optional NFS support */
#define	SYS_async_daemon 159
#define	SYS_getfh	160
#define	SYS_quotactl	161	/* optional disk quota support */
#define	SYS_sigreturn	162	/* 162 is used with signals only */
#define	SYS_adjrst	163	/* Adjust resident set size */

/*
 * SYS_BSD and SYS_ATT define the latest release of system-call numbers for
 * BSD and ATT systems, respectively; this is placed in the high-order part
 * of the system-call number so the kernel knows which system call handler to
 * use (see conf/conf_syscalls.c).  Thus binaries linked with older
 * system-call interfaces can be executed if the kernel is configured for it.
 *
 * The SYS_{BSD,ATT} value is in the high-order 16-bits of the system-call
 * number.  The system-call index is in the low 16-bits of the system-call
 * number.
 *
 * SYS_IDX() returns the system-call index value, independent of which
 * "release" is encoded.
 *
 * Current system-call release values:
 *	v2.1 and earlier:	0.
 *	v3.0 BSD and ATT SVAE:	1.
 *	v3.1 Dynix/SysV:	2.
 */

#define	SYS_IDX(sysnum)	((sysnum) & 0xFFFF)

#define	SYS_BSD		0x1
#define	SYS_ATT		0x2
