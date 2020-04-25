.\" $Copyright:	$
.\" Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
.\" Sequent Computer Systems, Inc.   All rights reserved.
.\"  
.\" This software is furnished under a license and may be used
.\" only in accordance with the terms of that license and with the
.\" inclusion of the above copyright notice.   This software may not
.\" be provided or otherwise made available to, or used by, any
.\" other person.  No title to or ownership of the software is
.\" hereby transferred.
...
. \" $Header: d.t 1.42 91/02/12 $
.Ct APPENDIX D "Kernel Tuning Parameters"
.Pa
The file
.I /sys/conf/param.c
contains the definitions of
almost all compile-time tuning parameters.
.I Config
copies this file into the directory of the system
being configured to allow
configuration-dependent rules and values to be maintained.
Some of these tuning parameters
can be specified in the kernel configuration file.
Others must be modified by editing
.I param.c .
Those parameters not mentioned here should not be altered for a binary
configuration.
.Bh "Parameters that Options Entries can Define"
.Pa
The following parameters can be defined using the
.I options
entries in the kernel configuration file,
as in the following example:
.Ps
options	TIMESHARE NMOUNT=24,MAXUPRC=20,SVSEMA
.Pe
.Pa
Note that the names of these variables in
.I /sys/conf/param.c
are lower-case,
but the corresponding options in the kernel configuration file
must be upper-case.
That is because the default values for these variables are defined
in the following manner in
.I /sys/conf/param.c :
.Ps
#ifndef NMOUNT
#define NMOUNT 64
#endif
 :
int nmount = NMOUNT;
.Pe
.Pa
The following three parameters \(em
.B COMMERCIAL ,
.B PARALLEL ,
and
.B TIMESHARE
\(em establish a number of system parameters.
Note that
.B K
is defined as 1024 in
.I param.c
and should not be changed or unpredictable results may occur.
.Ls H
.Li "\f3COMMERCIAL\fP"
This sets system heuristics for many users with few processes
working on many small files.
The processes are small and share a small number of
executable binaries.
Therefore, there is a greater use of file-locking and more buffer cache
usually yields greater performance;
the binaries share many pages, so a higher
minimum resident set is reasonable.
.Li "\f3PARALLEL\fP"
This sets system heuristics for a small number of users,
.B maxusers
at most 32,
each potentially using many large processes.
Therefore, the buffer size isn't as critical as available free
memory for the parallel program(s);
little or no use of file-record-locking is made.
Such systems may not be suitable for large parallel makes, since they
are tuned for large shared-memory programs (few large processes).
.Li "\f3TIMESHARE\fP"
This sets system heuristics for a large number of users with a moderate number
of medium size processes each, working on mostly unrelated files.
Therefore, little use of file-record-locking is made, and memory consumption
by processes outweighs the buffer cache.
.Li "\f3AVG_SIZE_PROCESS\fP"
The average size of a process (in Kbytes),
including the text, private data/bss, shared data/bss, and stack
segments.
The default value is 400,
64*K,
and 512 for
.B COMMERCIAL ,
.B PARALLEL ,
and
.B TIMESHARE ,
respectively.
.Li "\f3BUFPCT\fP"
The percentage free space allocated to buffer cache.
The default value is 20,
10,
and 10 for
.B COMMERCIAL ,
.B PARALLEL ,
and
.B TIMESHARE ,
respectively.
.Li "\f3DEFSSIZ\fP"
The maximum stack size of a process (in Kbytes).
The default value is 1024.
.Li "\f3MAXNOFILE\fP"
The maximum number of open file descriptors that one process
can have at any time.
If you want a process to have more file descriptors,
you must set
.B MAXNOFILE
before your program can use the
.I setdtablesize (2)
system call to increase the number of open file descriptors.
.B Note:
Since standard I/O does not support more than 20 open files,
all stdio must be done with file descriptor numbers less than 20.
The default value is 64.
.Li "\f3MAXSYMLINKS\fP"
The maximum number of symbolic links that can be processed
when interpreting a pathname.
This variable is used to halt infinite loops of symbolic links.
The default value is 8.
.Li "\f3MAXUPRC\fP"
The maximum number of processes any given uid
can be executing at any time,
other than uid 0,
.B root .
.B MAXUPRC
should be at least 10 if non-root processes are expected to run.
The default value is 100.
.Li "\f3NCALLOUT\fP"
The number of
.I callout
structures.  One
.I callout
structure is used per internal system event handled with a timeout.
Timeouts are used for terminal delays,
watchdog routines in device drivers,
and protocol timeout processing.
The default value is (16+\f3NPROC\fP).
.Li "\f3NCLIST\fP"
The number of
.I c-list
structures.
.I C-list
structures are used in terminal I/O.
The default value is (100+16*\f3MAXUSERS\fP)
.Li "\f3NDEVNODE\fP"
The number of VFS device vnodes available to the system.
One vnode is used for each block device opened since DYNIX
was booted.
If this value is set to 0,
the system computes this value based on the hardware configuration.
The default value is 0.
.Li "\f3NFILCK\fP"
The System V record locking limit for the maximum number
of locks available at any time.
This is a system-wide limit.
The default value is (\f3NFLINO\fP*\f3FILCK_MULT\fP).
.Li "\f3NFILE\fP"
The number of
.I "file table"
structures.
One file table structure is used for each open, unshared, file descriptor.
Multiple file descriptors may refer to a single file table
entry when they are created through a
.I dup
call, or as the
result of a
.I fork .
The default value is
(\f3FILE_MULT\fP*(\f3NPROC\fP+16+\f3MAXUSERS\fP)/\f3FILE_DIV\fP+32+
2*\f3NETSLOP\fP).
.Li "\f3NFLINO\fP"
The the maximum number of files that can use System V record locking at any
time.
This is a system wide limit.
The default value is ((\f3MAXUSERS\fP*\f3FLINO_MULT\fP)+50).
.Li "\f3NINODE\fP"
The maximum number of files in the file system that may be active at any time.
This includes files in use by users,
as well as directory files being read or written by the system
and files associated with bound sockets in the UNIX IPC domain.
The default value is
((\f3INODE_MULT\fP*\f3NPROC\fP)/\f3INODE_DIV\fP+\f3MAXUSERS\fP+48).
.Li "\f3NMBCLUSTERS\fP"
The maximum amount of memory space allocated to networking
for clusters specified in Kbytes.
The default value is ((256*K)/\f3CLBYTES\fP).
.Li "\f3NMFILE\fP"
The maximum number of mapped file descriptors that can be used
at any time by the system.
Mapped file space can be treated in much the same way as
.B NINODE .
The default value is (\f3MFILE_MULT\fP*\f3NPROC\fP/\f3MFILE_DIV\fP).
.Li "\f3NMOUNT\fP"
The maximum number of mounted file systems,
including the root file system,
at any point in time.
.B NMOUNT
must be greater than 0.
The default value is 64.
.Li "\f3NOFILETAB\fP"
The number of open-file table objects that are allocated at boot time.
The default value is
.B NPROC .
.Li "\f3NPROC\fP"
The maximum number of processes that may be running at any time.
The default value is (20+\f3PROC_MULT\fP*\f3MAXUSERS\fP).
.Li "\f3NUCRED\fP"
The number of VFS user-credential structures available to
the system.
The default value is
.B NPROC .
.Li "\f3RESPHYSMEM\fP"
The amount of system memory, in multiples of
.B CLBYTES ,
to reserve for custom devices such as accelerators and graphics frame
buffers.
The specified amount of memory
is preallocated and is never used by the system for kernel or user memory.
A message 
is printed which shows the address range that has been reserved for the
custom device.
The kernel variable
.I topmem
contains the physical address,
which is the beginning of the pre-allocated memory.
The default value is 0.
.Li "\f3INCR_PTSIZE\fP"
The amount to increase Usrptmap[] by.
The default is no extra entries.
.Li "\f3NUMBMAPCACHE\fP"
The number of buffer map caches.
The defualt number is 50.
.Li "\f3SVACCT\fP"
This enables the System V accounting facility.
The default value is 0.
.Li "\f3SVCHOWN\fP"
This enables the System V semantics on the
.I chown (2)
system call in the
.B att
universe.
The default value is 0.
.Li "\f3SVMESG\fP"
This enables the the System V message facility.
.Li "\f3SVSEMA\fP"
This enables the System V semaphore facility.
.Le
.Bh "Parameters Enabled by the SVMESG Option"
.Pa
The following parameters of the System V message facility are
configured only if the
.B SVMESG
option is specified in
the kernel configuration file:
.Ls H
.Li "\f3MSGMAP\fP"
Used to manage message buffer space in chunks specified by
.B MSGSSZ
bytes for the System V message facility.
The default value is 100.
.Li "\f3MSGMAX\fP"
The maximum message size in bytes for a System V message. 
.Li "\f3MSGMNB\fP"
The maximum number of bytes that can by used by a message queue. 
.Li "\f3MSGMNI\fP"
The maximum number of message queue identifiers.
.Li "\f3MSGSEG\fP"
The maximum number of message segments.
.Li "\f3MSGSSZ\fP"
The size of a unit of allocation for a message buffer.
.Li "\f3MSGTQL\fP"
The maximum number of system message queue headers.
There can be no more than this number of messages queued at any time.
.Li  "\f3Bmsg[],\ msgmap,\ msgque,\ msgh,\ msginfo\fP"
Miscellaneous tuning parameters.
These should not be changed.
.Le
.Bh "Parameters Enabled by the SVSEMA Option"
.Pa
The following parameters of the System V semaphore facility are
configured only if the
.B SVSEMA
option is specified in the kernel configuration file:
.Ls H
.Li "\f3SEMAEM\fP"
The maximum adjust on exit value.
.Li "\f3SEMMAP\fP"
The number of entries in the semaphore map used for the System V
semaphore facility.
.Li "\f3SEMMNI\fP"
The number of semaphore identifiers.
.Li "\f3SEMMNS\fP"
The maximum number of system wide semaphores.
.Li "\f3SEMMNU\fP"
The number of
.I undo
structures in the system.
.Li "\f3SEMMSL\fP"
The maximum number of semaphores any given uid can use.
.Li "\f3SEMOPM\fP"
The maximum number of semaphore operations allowed per
.I semop
call.
.Li "\f3SEMUME\fP"
The maximum number of
.I undo
entries per process.
.Li "\f3SEMVMX\fP"
The maximum value a semaphore can take.
.Li "\f3sema,\ sem,\ semmap,\ sem_undo,\ SEMUSZ,\ semu,\ semtmp,\ seminfo\fP"
Variables based on information provided for System V semaphore operations.
These should not be changed.
.Le
.Bh "Parameters Enabled by the QUOTA Option"
.Pa
The following parameters of the disk quota facility are
configured only if the
.B QUOTA
option is specified in the kernel configuration file:
.Ls H
.Li "\f3NDQUOT\fP"
The maximum number of disk quota structures
that can be active at any time.
For each filesystem
that has quotas enabled,
there must be a disk quota structure
for every user who has quota limits and 
who is actively using files.
The default value is
(\f3NINODE\fP+(\f3MAXUSERS\fP*\f3NMOUNT\fP)/4).
.Li "\f3NDQHASH\fP"
The number of hash table entries for the disk quota structures.
The default value is
((\f3NDQUOT\fP)/3).
.Li "\f3DQ_FTIMEDEFAULT\fP"
The amount of time a user has before the soft limits for
inodes are treated
as hard limits (usually resulting in an allocation failure). 
The time is
specified in seconds.  The default
value is (7*24*60*60) or one week.
.Li "\f3DQ_BTIMEDEFAULT\fP"
The amount of time a user has before the soft limits
for disk space are treated
as hard limits (usually resulting in an allocation failure). 
The time is
specified in seconds.  The default
value is (7*24*60*60) or one week.
.Bh "Parameters Config Cannot Alter"
.Pa
To change any of the following parameters,
you must edit
.I /sys/conf/param.c
or a copy of
.I param.c
that has been created by
.I config ,
such as
.I /sys/STARTUP/param.c .
Several parameters are described in the manual pages for 
.I vmtune (8)
and
.I vm_ctl (2).
The System V message and semaphore services are also tunable.
.Pa
These are the only
.I param.c
variables you should change:
If you don't see a
.I param.c
variable listed here,
you should not change its value.
.Ls H
.Li "\f3dk\ [\f2number\f3]\f1"
.I Number
is the number of structures to keep for disk-drive-performance statistics.
The default is 16.
.Li "\f3forkmapmult,\ forkmapdiv\fP"
The fraction
.B forkmapmult/forkmapdiv
determines the maximum amount of memory that
the system dedicates to concurrent forks.  This avoids
memory deadlocks and excessive memory committed to processes
executing the
.I fork
system-call,
which can not be swapped in this implementation.
The effect is to provide a rational
fraction.  The fraction should be less than one;
.B forkmapdiv
must be greater than 0.
The default is
.B forkmapmult
= 2,
.B forkmapdiv
= 3,
so that two-thirds of the initial free space
can be committed to concurrent forks.
This has no effect on execution of the
.I vfork
system call.
.Li "\f3light_show\fP"
Determines whether the system toggles the per-processor
LEDs on the dual-processor board(s) and/or front panel when present.
If this flag is non-zero, each processor turns its LED off on entering
the idle loop, and turns the LED on on exiting from this loop.
The default value is one.
.B Light_show
is interpreted as follows:
.Le
.KS
.TS
center tab(!) box ;
l | c   s   s
l | _   _   _
l | c | c | c .
Location of LEDs!Value of \f3light_show\fP
\^
\^!0!1!2
_
B8 processor boards!off!on!on
B21 processor boards!off!off!on
B21/S81 front panel!off!on!on
Symmetry processor boards!on!on!on
.sp .5
.TE
.KE
.Ls H
.Li "\f3maxRS\fP"
The maximum allowed size for any process's resident set,
in page clusters.
All requests to increase
resident set sizes (system-wide or per-process) are limited by
this value.  The system modifies this value during
initialization, if necessary, to ensure that a process with this
size resident set will fit in physical memory.  The default
value is 16 Mbytes/\f3CLBYTES\fP; typically the system reduces this to
fit physical memory.
A practical lower limit is about 100\(mu1024/\f3CLBYTES\fP.
.Li "\f3maxRSslop\fP"
A value used during system initialization to
determine an appropriate limit for
.B maxRS .
.B MaxRSslop
is specified in units of machine page size.
This value should be at least 40;
values beyond 200 are not likely to be useful.
The default value is 20\f3K\fP/\f3NBPG\fP.
.Li "\f3memintvl\fP"
The number of ticks between polls of soft errors on the memory boards.
This value should be greater than 0.
The default is 10\(mu60\(mu\f3HZ\fP (10 minutes).
.Li "\f3nbuf,\ bufpages\fP"
These values determine the size of the system's cache of disk
blocks.
The variable
.B bufpages
determines the number of page clusters,
each containing
.B CLSIZE
(the system page size) hardware pages,
reserved for the buffer cache.
The system manages these pages among a set of
.B nbuf
buffer headers to accommodate the variable-size blocks in the DYNIX
file system.
By default,
.B nbuf
and
.B bufpages
are initially 0;
the system computes them during system initialization to match the hardware
configuration.
The variable
.B bufpages
should be set to 0 or at least 32,
and 
.B nbuf
should be set to 0 or equal to
.B bufpages .
Too few
.B bufpages
or buffer headers adversely affect system performance.  Too
many can limit the system's free memory for user processes.
.Li "\f3root_prio_noage\fP"
Determines whether non-root processes can successfully
call proc_ctl(PRIO_AGE,pid,0) to keep a process from priority aging.
This is a boolean variable:
if it is set to non-zero,
only root processes can alter their ability to age.
If it is set to 0,
any process can alter its ability to age.
The default value is 1.
.Li "\f3root_vm_pffable\fP"
Determines whether non-root processes can successfully
call vm_ctl(VM_PFFABLE,0) to turn off modification of the
process's resident set size according to the process current
page fault frequency.
This is a boolean variable:
if it is set to non-zero,
only root processes can use this function.
If it is set to 0,
any process can use this function.
The default value is 1.
.Li "\f3root_vm_setrs\fP"
Determines whether non-root processes can successfully
call vm_ctl(VM_SETRS) to adjust their resident-set size.
This is a boolean variable:
if it is set to non-zero,
only root processes can alter their resident-set size.
If it is set to 0,
any process can alter its resident-set size.
The default value is 1.
.Li "\f3root_vm_swapable\fP"
Determines whether non-root processes can successfully
call vm_ctl(VM_SWAPABLE,0) to keep the process from swapping.
This is a boolean variable:
if it is set to non-zero,
only root processes can alter their swappability.
If it is set to 0,
any process can alter its swappability.
The default value is 1.
.Li "\f3ummap_max_hole\fP"
The maximum size, in hardware pages, of an unmapped region
associated with a
.B u_mmap
structure.
If two portions of a mapped file are more than
.B ummap_max_hole
pages apart, they are mapped using separate u_mmap structures.
The default value is 64\f3K\fP/\f3NBPG\fP pages.
.Li "\f3use_data_mover\fP"
(\f3B21 only\fP.)
Specifies whether the system's data mover is to be used
to accelerate block data transfers across the system bus.
If
.B use_data_mover
is non-zero, the data mover is used.
The default value is one.
.Li "\f3wdt_timeout\fP"
Sets the watchdog timeout that coordinates between
the kernel and the front-panel error light.  This value is
the number of seconds before the error light
comes on and the system is thought to be dead.  The kernel
communicates with the SCED firmware every 0.5 seconds.
This value should be at least 1.
The default is 10 seconds.
.Li "\f3cmask\fP"
Sets the defualt value of u_u_cmask for new processes. This should normaly
be left to its defualt value of 0.
.Le
.Tc
