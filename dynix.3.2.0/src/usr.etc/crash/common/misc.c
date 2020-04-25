/* $Header: misc.c 2.17 1991/07/26 18:35:05 $ */

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

/*
 * $Log: misc.c,v $
 *
 *
 *
 *
 *
 *
 */

#ifndef lint
static char rcsid[] = "$Header: misc.c 2.17 1991/07/26 18:35:05 $";
#endif

#include "crash.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dir.h> 
#ifdef _SEQUENT_
#include <signal.h>
#include <sys/resource.h>
#include <sys/timer.h>
#endif
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>
#ifdef CROSS
#include "/usr/include/sys/stat.h"
#else
#include <sys/stat.h>
#endif

#ifdef BSD
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <machine/plocal.h>
#include <sdb.h>
#include <stand/dump.h>
#else
#include "dump.h"
#endif

#include <a.out.h>
#include <sys/time.h>
#include "compact.h"

#ifdef _SEQUENT_
#include <ldfcn.h>
#endif

long Etext;
long End;
int disrd;
int xdebug;
extern char is_compressed;
extern unsigned int vtop();
static int upyet;
static unsigned int maxkmem;

cmdsum()
{
	register  struct  tsw	*tp;
	char lbuf[128];

#ifdef BSD
	printf("Dynix Dump Analyzer -- Version %s of %s.\n\n", 
							Version, Date);
#else
	printf("DYNIX/ptx(R) Dump Analyzer -- Version %s of %s.\n\n",
							Version, Date);
#endif
	printf("available commands:\n");
	for(tp = t; tp->t_snm; tp++)
		if(tp->t_dsc) {
			if (*tp->t_snm == ' ')
				lbuf[0] = '\0';
			else
				sprintf(lbuf, "(%s)", tp->t_snm);
			printf("%s\t%s\t%s\n", lbuf, tp->t_nm, tp->t_dsc);
		}
	tok = 0;
}

nop(){}

char *
addr_str(addr)
{
	extern char *lookbyptr();
	extern unsigned int search();
	unsigned int off;
	char *symfound;
	static char buf[ MAXSTRLEN + 10 ];

	if (addr < 0x2000) {
		sprintf(buf, "%#x", addr);
		return buf;
	}
	if (End == 0)
		End = search("end");

	symfound = lookbyval(addr, &off);
	if (!symfound && (addr >End)) {
		symfound = lookbyptr(addr, &off);
		if (symfound && (off <= 0x1000)) {
			if( off )
				sprintf(buf, "(*%s)+%#x", symfound, off);
			else
				sprintf(buf, "(*%s)",symfound);
			return buf;
		}
	} else {
		if (symfound && (off <= 0x1000)) {
			if( off )
				sprintf(buf, "%s+%#x", symfound, off);
			else
				sprintf(buf, "%s",symfound);
			return buf;
		}
	} 
	sprintf(buf, "%#x", addr);
	return buf;
}

What()
{
	int  addr;
	char *arg;

	while((arg = token()) != NULL) {
		addr = atoi(arg);
		if ( err_atoi ) {
			printf("'%s', %s?\n", arg, err_atoi);
			continue;
		}
		printf("%s = %#x\n", addr_str(addr), addr);
	}
}

Vtop()
{
	int vaddr;
	int phys;
	char *arg;

	arg = token();
	vaddr = atoi(arg);
	phys = vtop(vaddr);
	printf("virt = %s, phys = %08#x\n", arg, phys);
}
	
unsigned int
vtop(vaddr)
	unsigned int vaddr;
{
	unsigned int paddr;
	
#ifdef	i386
	if (vaddr < 8192)
		return vaddr;
#endif
	if (!upyet)
		return vaddr;

	if (vaddr > maxkmem) {
		return(_vtop(vaddr));
	}
	paddr = (int)((int *)Sysmap + (vaddr / NBPG));
	if (is_compressed)
		core_read(paddr, (char *)&paddr , sizeof(paddr));
	else {
		lseek(fileno(vmcorefd), paddr, 0);
		read(fileno(vmcorefd), &paddr, sizeof paddr);
	}
	return (paddr & ~(NBPG-1)) | (vaddr & (NBPG-1));
}

readv(vaddr, addr, nbytes)
	unsigned int vaddr;
	char	*addr;
	int	nbytes;
{
	register int nbyt, nrd, n;
	unsigned int p;
	long loc;

	nbyt = nbytes;
#if CRASH_DEBUG
	if (xdebug > 1)
		printf("readv(vaddr=%#x, addr=%#x, nbytes=%d)\n", vaddr, addr, nbytes);
#endif
	while( nbytes ) {
		p = vtop(vaddr);
		n = NBPG - vaddr % NBPG;
		nrd = nbytes > n ? n : nbytes;
/* Need to check for transfer crossing over etext */
#ifdef PC_AT386
                /*
                 * If this is a PC (ie. EISA bus machine) the text segment
                 * in the core file is fine.  Don't bother to look in the 
                 * a.out file.  (yes, this could be shortened a bit,
		 *               but this seemed to be clearest code.)
                 */
                if (SYSTYP_PC(arch_type)) {
                        if (is_compressed)
                                n = core_read((long)p, addr, nrd);
                        else {
                                lseek(fileno(vmcorefd), (long) p, 0);
                                n = read(fileno(vmcorefd), addr, nrd);
			}
                } else {
		    /*
		     * text starts at 0x4000 for symmtery/balance on
		     * s16 s27 ... and b8 b80000 ... series
		     */
                    if (p < Etext && p > 0x4000) {
                        /* text of vmcore is trashed so read from /unix */
                        loc = (long) p - text_start + text_offset;
                        lseek(kernfd, (long) loc, 0);
                        n = read(kernfd, addr, nrd);
                    } else {
                        if (is_compressed)
                                n = core_read((long)p, addr, nrd);
                        else {
                                lseek(fileno(vmcorefd), (long) p, 0);
                                n = read(fileno(vmcorefd), addr, nrd);
                        }
                    }
                }
#else /* ! PC_AT386 */
	        /*
	         * text starts at 0x4000 for symmtery/balance on
	         * s16 s27 ... and b8 b80000 ... series
	         */
	        if (p < Etext && p > 0x4000) {
			/* text of vmcore is trashed so read from /unix */
			loc = (long) p - text_start + text_offset;
			lseek(kernfd, (long) loc, 0); 
			n = read(kernfd, addr, nrd);
		} else {
                        if (is_compressed)
                                n = core_read((long)p, addr, nrd);
                        else {
                                lseek(fileno(vmcorefd), (long) p, 0);
                                n = read(fileno(vmcorefd), addr, nrd);
                        }
		}
#endif /* PC_AT386 */
		if( n != nrd )
			return -1;
		nbytes -= nrd;
		vaddr  += nrd;
		addr   += nrd;
	}
	return nbyt;
}

readp(paddr, addr, nbytes)
{
#ifdef	CRASH_DEBUG
	if (xdebug > 5)
		printf("readp(paddr=%#x, addr=%#x, nbytes=%d\n", paddr, addr, nbytes);
#endif
	if (is_compressed)
		return core_read(paddr, addr, nbytes);
	else {
		lseek(fileno(vmcorefd), paddr, 0);
		return read(fileno(vmcorefd), addr, nbytes);
	}
}

int	err_search;

unsigned int
search(sym)
char *sym;
{
	int val;

	err_search = 0;
	if (lookbysym(sym, &val, N_GSYM) == FALSE) {
		err_search = 1;
		if (debug[18])
			printf("search('%s') failed\n", sym);
		return NULL;
	}
	return val;
}


/*
 * Read the sysmap and the memory map.
 */
getsysmap()
{
	int mmap;
	int msize;
	int i;

	readp(search("Sysmap"), &Sysmap, sizeof Sysmap);
	readp(search("upyet"), &upyet, sizeof upyet);
	readp(search("maxkmem"), &maxkmem, sizeof maxkmem);
	readp(CD_LOC, &Cd_loc, sizeof Cd_loc);
	/* int is 4 bytes on balance and symmetry */
	msize = Cd_loc.c_mmap_size/MC_BPI*4; 		
	mmap = (int)Cd_loc.c_mmap;
#ifdef BSD
	Cd_loc.c_mmap = (u_long *)malloc(msize);
#else
	Cd_loc.c_mmap = (ulong *)malloc(msize);
#endif
	readp(mmap, Cd_loc.c_mmap, msize);
}

/*
 * addr_exist()
 *      Return true iff HW addr "addr" exists in physical memory.
 */

addr_exists(addr)
	register int addr;
{
	struct config_desc *c = &Cd_loc;

	addr /= MC_CLICK;
	return((addr >= Cd_loc.c_mmap_size) ? 0 : MC_MMAP(addr, c));
}

#ifdef _SEQUENT_
getv() {
	 extern struct	var v;
	 readv(search("v"), &v, sizeof v);
}
#endif

trace()
{
	char *c;

	c = token();
	if (c == NULL) {
		if (xdebug) 
			xdebug = 0;
		else
			xdebug = 1;
	} else 
		xdebug = atoi(c);
	printf("Debug trace set to %d\n", xdebug);
	tok = 0;
}

Debug()
{
	set_debug("-D");	/* any string would be ok */
	if (debug[14])
		check_dup(1);
	tok = 0;

}

char *
nice_char(c)
	register char c;
{
	static char lbuf[4];
	char lb[4];

	lbuf[0] = 0;
	if (c == 0 || c == '\t') {
		sprintf(lbuf, "%c", c);
		return (lbuf);
	}
	if (c & 0200) {
		sprintf(lbuf, "M-");
		c &= 0177;
	}
	if (c < ' ') {
		sprintf(lb, "^%c", c+'@');
		strcat(lbuf, lb);
	} else if (c == 0177) {
		strcat(lbuf, "^?");
	} else {
		sprintf(lb, "%c", c);
		strcat(lbuf, lb);
	}
	return(lbuf);
}

char *
user_cmd(u)
	register struct user *u;
{
	register i;
	register unsigned char *s;
	static char lbuf[3*MAXCOMLEN];

	s = (unsigned char *) u->u_comm;
	lbuf[0] = 0;
	for (i=0; i < MAXCOMLEN && s[i]; i++)
		strcat(lbuf, nice_char(s[i]));
	return lbuf;
}

Dis()
{
	char *arg;
	int addr, count, length;
#ifdef _SEQUENT_
	extern char symrep[];
	char	*pp;
#endif

	if((arg = token()) == NULL) {
		printf("symbol expected\n");
		return;
	}
	addr = atoi(arg);
	if ( err_atoi ) {
err1:		printf("'%s', %s?\n", arg, err_atoi);
		tok = 0;
		return;
	}
	if((arg = token()) == NULL) {
		count = 10;
		disrd = 0;
		goto doit;
	} else {
		count = atoi(arg);
		if ( err_atoi ) {
			goto err1;
		}
	}

	if ((arg = token()) == NULL)
		disrd = 0;	/* read virtual */
	else if (*arg == 'p')
		disrd = 1;	/* read physical */
	else if (*arg == 'v')
		disrd = 0;	/* read virtual */
doit:
	while( count -- > 0 ) {
#ifdef	ns32000
		char *p = (char *)dis32000(addr, &length);
#endif
#ifdef	i386
		char *p = (char *)dis386(addr, &length);
#endif
		printf("%-19s%s	", addr_str(addr), disrd ? "p" : "v");
		if( length ) {
#ifdef _SEQUENT_
			if (*symrep && strcmp(&p[7], symrep)) {
				printf("%-30s", p);
				p[7] = '\0';
				printf("%s%s\n", p, symrep);
			} else {
				printf("%s\n", p);
			}
#else
			printf("%s\n", p);
#endif
			addr += length;
		} else {
			printf("%#0x\n", Disread(addr));
			addr++;
		}
	}
}

Disread(addr)
char *addr;
{
	unsigned char byte;

	if (disrd)
		readp(addr, &byte, sizeof byte);
	else
		readv(addr, &byte, sizeof byte);
	return( byte );
}

#ifdef _SEQUENT_
Dislook(addr)
{
	printf("%s:", addr_str(addr));
}

Dislocal(val, regno, sp)
	long	val;
	int	regno;
	char	**sp;
{
	int	i;
	extern	char	**regname;

	if (regno == 5) { 	/* ebp */
		if (val >= 8) {
			i = (val-8)/4;
			*sp += sprintf(*sp, "0x%x(%s) <arg #%d>", 
				val, regname[regno], i);	
			return;
		}
	}
	*sp += sprintf(*sp, "0x%x(%s)",  val, regname[regno]);
}
#endif

static short *seg_map;		/* maps a segment to a buffer if cached */
#ifdef _SEQUENT_
extern struct cdump_info header;
#else
extern struct dump_info header;
#endif
static unsigned next_free = 0;	/* point to next buffer to be used */
static unsigned first_free = 0;	/* point to first general  buffer */

typedef  struct buffer {
	int  seg_no;		/* uncompressed data corresponding to this
				 * seg_no is cached in this buffer
				 */

	char *data_p;		/* pointer to the uncompressed data */
} CACHE;

#define RETRY_SZ (0.5*1024*1024)

static CACHE *cache_ptr = NULL;
static cache_sz;
static num_segs;

/* setup to understand compressed dumps */

core_setup()
{
	if (readhdr() != -1)
		is_compressed = 1;	
	else
		return;
	seg_map = (short *)(malloc(header.comp_size));
	num_segs = header.comp_size /(sizeof(int)/sizeof(char)) - 1;
	if ((cache_sz = alloc_cache()) == 0){
		printf("no space for buffers\n");
		exit(1);
	}
#ifdef DEBUG
	printf("num of segs in cache = %d\n",cache_sz);
#endif /* DEBUG */
	map_init(num_segs);

}

/* pre_read PERM_CACHE amount of data starting at _proc. This amount is
 * wired down in the cache. 
 */
pre_read()
{
	register unsigned num;
	register unsigned i;


	if (is_compressed) {
		i = (int)proc / header.seg_size;
		num = PERM_CACHE / header.seg_size;
		if (PERM_CACHE % header.seg_size)  
			num++;
		while (num) {
			if (seg_map[i] < 0){
				if (decompress(i,(cache_ptr + next_free)->data_p) != -1){
					seg_map[i] = next_free;
					(cache_ptr + next_free++)->seg_no = i;
					num--;
				}
			}
			i++;
		}
		first_free = next_free;
	}
}

/* read num_bytes at offset  into buf. If the segment corresponding to 
 * offset is already uncompressed and cached, just copy it from the buffer.
 * If this seg. is not cached, uncompress it and cache.
 */

core_read(offset, buf, num_bytes)
unsigned offset;
char *buf;
int num_bytes;
{
	char *cache_addr, *read_ptr;
	unsigned read_amount = 0;
	unsigned last_read = 0;
	unsigned cur_seg_bytes;
	unsigned seg_size = header.seg_size;
	int seg = offset/seg_size;
	int seg_offset = offset % seg_size;
	int total_read = 0;
	int size;
	char partial_read = 0;
	while (num_bytes){

		cur_seg_bytes = seg_size - seg_offset;
		last_read = read_amount = MINS(num_bytes, cur_seg_bytes);
		if (seg >= num_segs)
			return(total_read);
		if (seg_map[seg] < 0){       
			/*seg needs to be uncompressed*/
			cache_addr = (cache_ptr + next_free)->data_p;
			if ((size = decompress(seg, cache_addr)) == -1){
				return(total_read);
			}
			if ( size != seg_size)
				partial_read = 1;
			if ((cache_ptr + next_free)->seg_no >= 0)
				seg_map[(cache_ptr + next_free)->seg_no] = -1;
			seg_map[seg] = next_free;
			(cache_ptr + next_free)->seg_no = seg;
			if (++next_free == cache_sz){
				next_free = first_free;
			}
		
		}else   /*uncompressed data in cache*/
			cache_addr = (cache_ptr + seg_map[seg])->data_p;		
	
		if (partial_read){
			if (size > seg_offset){
				last_read = read_amount = MINS(num_bytes,
				size - seg_offset);
			}else
				return(total_read);
		}
		read_ptr = cache_addr + seg_offset;
		while (read_amount--){   
			*buf++ = *read_ptr++;
		}

		num_bytes -= last_read;	
		total_read += last_read;

		if (num_bytes ){
			if (partial_read)
				return(total_read);
			seg_offset = 0;
			seg++;
		}
	}
	return(total_read);
}

/* Try to allocate a cache of size CACHE_SZ. If malloc fails, retry with
 * a size RETRY_SIZE smaller. The cache should be atleast PERM_CACHE big.
 */
alloc_cache()
{
	unsigned max_bufs;
	int cache_size;
	int num_bufs;
	CACHE * ptr;
	int i;
	caddr_t cache_start,next;
	caddr_t cache_end;

	max_bufs = CACHE_SZ / (sizeof(CACHE) + header.seg_size);
	num_bufs = MINS(num_segs,max_bufs);
#ifdef DEBUG
	printf("num_buf = %d\n",num_bufs);
#endif /* DEBUG */

	if ((cache_ptr = (CACHE *)sbrk(num_bufs * sizeof(CACHE))) == NULL)
		return(0);	
	cache_size = num_bufs * header.seg_size;
	while ((cache_start = (caddr_t)sbrk(cache_size)) == (caddr_t)-1){
		cache_size -= RETRY_SZ;
		if (cache_size <= 0)
			return(0);
	}

	cache_end = (caddr_t)sbrk(0);
	next = cache_start;
	for (i=0,ptr = cache_ptr;i < num_bufs; i++,ptr++){
		if (next <= cache_end){
			ptr->seg_no = -1;
			ptr->data_p = next;
			next += header.seg_size;
		}
		else{
			num_bufs = i;
			if (num_bufs*header.seg_size < PERM_CACHE+LOW_WATER)
				return(0);
			else
				return(num_bufs);
		}
	}
	return(num_bufs);

}

/* Initialize the seg_map to show all segments to be uncached */
static
map_init(size)
register unsigned size;
{
	short *ptr = seg_map;
	while(size--)
		*ptr++ = -1;
}
