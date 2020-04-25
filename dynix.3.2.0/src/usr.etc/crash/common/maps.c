#ident "$Header: maps.c 1.5 1991/07/26 20:44:25 $"

/*
 * $Copyright: $
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
 * $Log: maps.c,v $
 *
 *
 *
 */

#ifndef lint
static char rcsid[] = "$Header: maps.c 1.5 1991/07/26 20:44:25 $";
#endif

#include "crash.h"

#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/cmap.h>
#include <sys/dir.h>
#ifdef _SEQUENT_
#include <sys/resource.h>
#else
#include <sys/kernel.h>
#undef TRUE
#undef FALSE
#undef CMASK
#endif
#include <signal.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <curses.h>

static int	dflag;			/* 0 no display
					 * 1 single display
					 * 2 first of continous display
					 * 3 continous display
					 */
static unsigned int	low_addr;
static unsigned int	high_addr;
static unsigned int	scale;
static unsigned int	subscale;		
static int	d_rows = 22;
static int	d_cols = 64;
static int	pixs = 8;
static unsigned int	max_disp;
static int	scaled;			/* flag to indicat change of scale */

static char	*map_disp = NULL;
static char	*tmp_disp = NULL;


/*
 * map excepts a resource map or cmap followed by an optinal -d or -D
 * The -d option will result in a graphic table. -D is the same excpet
 * the result of a previous "map" will be accumulated.
 * An optional rows and columns figure may also follow -d or -D
 */
map()
{
	register int i;
	char *arg;
	char *opt;
	int	addr;
	struct	sdb *pp;
	extern	void pr_rmap();
	extern	void pr_cmap();
	extern	void pr_mem();
	static	int	map_init = 0;

	if (map_init == 0) {
		map_init = 1;
#ifndef _SEQUENT_
		savetty();
#endif
		initscr();
#ifdef _SEQUENT_
		reset_shell_mode();
#endif
		d_rows = LINES-3;
		d_cols = COLS;
	}

	if((arg = token()) == NULL) {
		printf("symbol expected\n");
		return;
	}
	addr = ato_addr(arg, 1);
	if ( err_atoi ) {
		printf("'%s', %s?\n", arg, err_atoi);
		tok = 0;
		return;
	}

	/*
	 * check args.
	 */
	if((opt = token()) != NULL) {
		if (strcmp(opt, "-d") == 0 ) {
			dflag = 1;
		} else if (strcmp(opt, "-D") == 0 ) {
			if (dflag < 2)
				dflag = 2;
			else
				dflag = 3;
		}
		if ((opt = token()) != NULL) {
			if (i = atoi(opt))
				d_rows = i;
			if ((opt = token()) != NULL) {
				if (i = atoi(opt))
					d_cols = i;
			}
		}
	} else {
		dflag = 0;
	}


	if (dflag) {
		max_disp = d_rows * d_cols * pixs;

		if (map_disp == NULL) {
			map_disp = malloc((max_disp/8)+1);
			tmp_disp = malloc((max_disp/8)+1);
		} else {
			map_disp = realloc(map_disp, (max_disp/8)+1);
			tmp_disp = realloc(tmp_disp, (max_disp/8)+1);
		}
		if (dflag != 3) {
			/*
			 * reset display map.
			 */
			low_addr = -1;
			high_addr = 0;
			scale = 1;
			subscale = 1;
			for (i = 0; i < max_disp/8; i++)
				map_disp[i] = 0;
			clear();
		}
	}

	if (strcmp(arg, "cmap") == 0 ) {
		pr_cmap((struct cmap *)addr);
	} else if (strcmp(arg, "c_mmap") == 0) {
		pr_mem();
	} else  {
		pr_rmap((struct map *)addr);
	}
	if (dflag) {
		disp_map();
#ifdef _SEQUENT_
		reset_shell_mode();
#else
		resetty();
#endif
	}
}


/*
 * print a resource map.
 * A resource map is a "struct map" followed by one or more "struct mapent"s
 */
static void
pr_rmap(map)
	struct map	*map;
{
	struct map mp;
	char	name[20];
	register struct mapent *ep;
	register struct mapent *bp;
	struct mapent	me;
	unsigned int	last;
	unsigned int	limit;

	readv((int)map, &mp, sizeof mp);
	if (mp.m_name == 0) {
		printf("Map has not been initialised\n");
		return;
	}
	readv(mp.m_name, name, 20);
	ep = (struct mapent *)(map+1);
	limit = mp.m_limit - ep;

	if (dflag < 2) {
		printf("%s: 0x%x - 0x%x (%d 0x%x)\n", 
			name, ep, mp.m_limit, limit, limit);
	}
	if (dflag && dflag < 3) {
		re_scale(limit);
	}

	last = 0;
	for (bp = ep; ;bp++) {
		readv(bp, &me, sizeof me);
		if (me.m_size == 0)
			break;
		if (live && (last >= me.m_addr))  /* race against rmfree() */
			continue;

		if (dflag) {
			if (me.m_addr != last)
				acc_map(last, me.m_addr - last);
		} else {
			if (me.m_addr != last)
				printf("%8.8x %8.8x Used\n",last, me.m_addr - last);
			printf("%8.8x %8.8x free\n",me.m_addr, me.m_size);
		}
		last = me.m_addr + me.m_size;
	}
}

/*
 * convert cmap type to text.
 */
static char *
cmap_type(type)
	int type;
{
	switch (type) {
	case CSYS:
		return("CSYS  ");
	case CSTACK:
		return("CSTACK");
	case CDATA:
		return("CDATA ");
	case CMMAP:
		return("CMMAP ");
	}
}


/*
 * Print out cmap entries.
 */
static void
pr_cmap(map)
	register struct cmap *map;
{
	register struct	cmap	*c;
	register unsigned int	addr;
	struct	cmap	cmap;
	struct	cmap	*ecmap;
	struct	cmap	cm_clean;
	struct	cmap	cm_dirty;
	unsigned int	mem_click;
	unsigned int	low_hole;
	unsigned int	high_hole;


	readv(search("ecmap"), &ecmap, sizeof ecmap);
	readv(search("cm_clean"), &cm_clean, sizeof cm_clean);
	readv(search("cm_dirty"), &cm_dirty, sizeof cm_dirty);

	if (dflag < 2)
		printf("cmap from 0x%8.8x - 0x%8.8x (0x%x)\n", 
			map, ecmap, (ecmap-map) * NBPG);
	if (dflag == 0)
		printf(" phys addr:  virt addr: ref cnt, waiter, owner, flags\n");
	else {
		re_scale((ecmap-map) * NBPG);
	}
	/*
	 * parts of the map are not really here, as meminit frees pages of
	 * memory theat lie within cmap - emap if those pages can not be
	 * used. This is true foe all adrreses befoe that of cmap itself
	 * and thos that map holes in memory.
	 */

	low_hole = -1;
	high_hole = 0;
	for (c = map; c < ecmap; c++) {
		addr = (c-map) * NBPG;
		if (!addr_exists(addr)) {
			if (addr > high_hole)
				high_hole = addr;
			if (addr < low_hole)
				low_hole = addr;
			continue;
		}
		if (addr < (unsigned int)map) {
			if (addr > high_hole)
				high_hole = addr;
			if (addr < low_hole)
				low_hole = addr;
			if (dflag)
				acc_map(addr, NBPG);
			continue;
		}
		if (high_hole && !dflag) {
			printf("0x%8.8x to 0x%8.8x unused\n", 
						low_hole, high_hole);
		}
		low_hole = -1;
		high_hole = 0;
		readv(c, &cmap, sizeof cmap);
		if (cmap.c_refcnt == 0)
			continue;
		if (dflag) {
			acc_map(addr, NBPG);
		} else {
			printf("0x%8.8x: 0x%8.8x:   %5u   %5u  %8.8x %s %c,%c,%c,%c,%c,%c\n",
				addr,
				cmap.c_page,
				cmap.c_refcnt,
				cmap.c_iondx,
				cmap.c_ndx,
				cmap_type(cmap.c_type),
				cmap.c_dirty	?'d':' ',
				cmap.c_holdfod	?'h':' ',
				cmap.c_pageout	?'p':' ',
				cmap.c_gone	?'g':' ',
				cmap.c_intrans	?'i':' ',
				cmap.c_iowait	?'w':' ');
		}
	}
}

/*
 * Print out memmory map
 */
static void
pr_mem()
{
	register int i;

	printf(" bytes  Mbytes: (. = %dKb)", MC_CLICK/(1024));
	for(i=0; i < Cd_loc.c_mmap_size; i++) {
		if ((i%64) == 0)
			printf("\n%8.8x(%4.4d):",i*MC_CLICK,
					i*MC_CLICK/(1024*1024));
		if (addr_exists(i*MC_CLICK))
			printf(".");
		else
			printf("X");
	}
	printf("\n");
}


/*
 * accumulate into a display map.
 * resize and rescale if needed.
 */

static
acc_map(addr, size)
	unsigned int	addr;
	unsigned int	size;
{
	unsigned int start;
	register unsigned int	i;
	register unsigned int	e;

	if (addr < low_addr) {
		re_scale(addr);
	} 
	if ((addr+size) > high_addr) {
		re_scale(addr + size);
	}
	start = ((addr - low_addr) * subscale)/ scale;
	e = start + (((size * subscale) + scale - 1 ) / scale);

	for (i = start; i < e; i++) {
		map_disp[i / 8] |= (1 << (i % 8));
	}
}


/*
 * Rescale the bit array to accomadate the new range
 */
static
re_scale(addr)
	register unsigned int addr;
{
	unsigned int	new_scale;
	unsigned int	new_low;
	unsigned int	new_high;
	unsigned int	new_subscale;
	register int	i;
	register int	j;

	if ((addr <= high_addr) && (addr >= low_addr))
		return;

	new_low = low_addr;
	new_high = high_addr;
	if (addr < low_addr) {
		new_low = (addr / pixs) * pixs;		/* round down */
	} 
	if (addr > high_addr) {
		new_high = ((addr / pixs)+1) * pixs;    /* round up */
	}

	/*
	 * Calculate new scale.
	 * Don't allow to go to zero.
	 */
	i = new_high - new_low;
	if ( i <= max_disp) {
		new_scale = 1;
		if (i > max_disp/100) {
			new_subscale = max_disp / i ;
		} else {
			new_subscale = 100;
		}
	} else {
		if ((new_scale = (i + max_disp -1 )/ max_disp) == 0) {
			new_scale = -1 / max_disp;
			new_low = 0;
			new_high = -1;
		}
		new_subscale = 1;
	}

	if ((new_scale == scale) && (new_low == low_addr) && 
					(new_subscale == subscale))
		return;

	/*
	 * now translate
	 */

	for (i = 0; i < max_disp / 8; i++)
		tmp_disp[i] = 0;

	for (i = 0; i < max_disp; i++) {
		if (map_disp[i / 8] & (1 << (i % 8))) {
			j = (((low_addr + ((i * scale) / subscale))
						  - new_low) * new_subscale)
				/ new_scale;
			tmp_disp[j / 8] |= (1 << (j % 8));
		}
	}
	memcpy(map_disp, tmp_disp, max_disp / 8);

	scale = new_scale;
	low_addr = new_low;
	high_addr = new_high;
	subscale = new_subscale;
	scaled = 1;
}


/*
 * Print out the display map.
 */
static
disp_map()
{
	register int	i;
	register int	j;
	char	*s;
	char	*term;
	static  char 	pixchar[] = " .,o*O@0#"; 
	int	p;
	int	c;

	/*
	 * calculate the occupancy.
	 */
	p = 0;
	c = (high_addr - low_addr)/scale;
	for (i = 0; i < c; i++) {
		if (map_disp[i / 8] & (1<<(i % 8)))
			p++;
	}
	if (c)
		p = (p * 100 )/c;
	else
		p = 0;

	move(0,0);
	if (scale > 1 || (subscale == 1))
		printw("0x%8.8x - 0x%8.8x (units:%d %3.3d%% full)\n",
			low_addr, high_addr, scale, p);
	else {
		printw("0x%8.8x - 0x%8.8x (units:0.%3.3d %3.3d%% full)\n",
			low_addr, high_addr, 1000/subscale, p);
	}
	

	for (i=0; i < max_disp; i += pixs) {
		c = i / pixs;
		p = 0;
		for (j = 0; j < pixs; j++) {
			if (map_disp[(i+j) / 8] & (1<<( (i+j) % 8)))
				p++;
		}
		if (p) {
			move( (c / d_cols) +1, c % d_cols);
			addch( pixchar[p] );
		}
	}
	move(d_rows+2, 0);
	refresh();
	/* wait to drain  fixes network flush out on flush read bug*/
	ttydrain();
	scaled = 0;
	return;
}
