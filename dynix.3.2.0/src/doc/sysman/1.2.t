.\" $Copyright:	$
.\" Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
.\" Sequent Computer Systems, Inc.   All rights reserved.
.\"  
.\" This software is furnished under a license and may be used
.\" only in accordance with the terms of that license and with the
.\" inclusion of the above copyright notice.   This software may not
.\" be provided or otherwise made available to, or used by, any
.\" other person.  No title to or ownership of the software is
.\" hereby transferred.
...
. \" $Header: 1.2.t 1.3 86/03/06 $
.\" %M% %I% %E%
.sh "Memory management\(dg
.NH 3
Text, data and stack
.PP
.FS
\(dg This section represents the interface planned for later
releases of the system.  Of the calls described in this section,
only \f2sbrk\fP and \f2getpagesize\fP are included in 4.2BSD.
.FE
Each process begins execution with three logical areas of memory
called text, data and stack.  
The text area is read-only and shared, while the data and stack
areas are private to the process.  Both the data and stack areas may
be extended and contracted on program request.  The call
.DS
addr = sbrk(incr);
result caddr_t addr; int incr;
.DE
changes the size of the data area by \f2incr\fP bytes and
returns the new end of the data area, while
.DS
addr = sstk(incr);
result caddr_t addr; int incr;
.DE
changes the size of the stack area.
The stack area is also automatically extended as needed.
On the VAX the text and data areas are adjacent in the P0 region,
while the stack section is in the P1 region, and grows downward.
.NH 3
Mapping pages
.PP
The system supports sharing of data between processes
by allowing pages to be mapped into memory.  These mapped
pages may be \f2shared\fP with other processes or \f2private\fP
to the process.
Protection and sharing options are defined in <mman.h> as:
.DS
._d
/* protections are chosen from these bits, or-ed together */
#define	PROT_READ	0x4	/* pages can be read */
#define	PROT_WRITE	0x2	/* pages can be written */
#define	PROT_EXEC	0x1	/* pages can be executed */

/* sharing types; choose either SHARED or PRIVATE */
#define	MAP_SHARED	1	/* share changes */
#define	MAP_PRIVATE	2	/* changes are private */
.DE
The cpu-dependent size of a page is returned by the
\f2getpagesize\fP system call:
.DS
pagesize = getpagesize();
result int pagesize;
.DE
.PP
The call:
.DS
mmap(addr, len, prot, share, fd, pos);
caddr_t addr; int len, prot, share, fd; off_t pos;
.DE
causes the pages starting at \f2addr\fP and continuing
for \f2len\fP bytes to be mapped from the object represented by
descriptor \f2fd\fP, at absolute position \f2pos\fP.  The parameter
\f2share\fP specifies whether modifications made to this mapped copy
of the page, are to be kept \f2private\fP, or are to be \f2shared\fP with
other references.
The parameter \f2prot\fP specifies the accessibility
of the mapped pages.
The \f2addr\fP, \f2len\fP, and \f2pos\fP parameters
must all be multiples of the pagesize.
.PP
A process can move pages within its own memory by using the
.I mremap
call:
.DS
mremap(addr, len, prot, share, fromaddr);
caddr_t addr; int len, prot, share; caddr_t fromaddr;
.DE
This call maps the pages starting at \f2fromaddr\fP to the address specified
by \f2addr\fP.
.PP
A mapping can be removed by the call
.DS
munmap(addr, len);
caddr_t addr; int len;
.DE
This causes further references to these pages to refer to private
pages initialized to zero.
.NH 3
Page protection control
.PP
A process can control the protection of pages using the call
.DS
mprotect(addr, len, prot);
caddr_t addr; int len, prot;
.DE
This call changes the specified pages to have protection \f2prot\fP\|.
.NH 3
Giving and getting advice
.PP
A process that has knowledge of its memory behavior may
use the \f2madvise\fP call:
.DS
madvise(addr, len, behav);
caddr_t addr; int len, behav;
.DE
\f2Behav\fP describes expected behavior, as given
in <mman.h>:
.DS
._d
#define	MADV_NORMAL	0	/* no further special treatment */
#define	MADV_RANDOM	1	/* expect random page references */
#define	MADV_SEQUENTIAL	2	/* expect sequential references */
#define	MADV_WILLNEED	3	/* will need these pages */
#define	MADV_DONTNEED	4	/* don't need these pages */
.DE
Finally, a process may obtain information about whether pages are
core resident by using the call
.DS
mincore(addr, len, vec)
caddr_t addr; int len; result char *vec;
.DE
Here the current core residency of the pages is returned
in the character array \f2vec\fP, with a value of 1 meaning
that the page is in-core.
