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

#ident "$Header: uncompress.c 1.1 1991/04/16 18:38:53 $"

/* 
 * decompress - data uncompression code 
 */

#define	min(a,b)	((a>b) ? b : a)
#define TRUE	1

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "compact.h"
#include <stand/dump.h>

#if BITS == 16
# define HSIZE  69001           /* 95% occupancy */
#endif
#if BITS == 15
# define HSIZE  35023           /* 94% occupancy */
#endif
#if BITS == 14
# define HSIZE  18013           /* 91% occupancy */
#endif
#if BITS == 13
# define HSIZE  9001            /* 91% occupancy */
#endif
#if BITS <= 12
# define HSIZE  5003            /* 80% occupancy */
#endif

typedef long int	code_int;

typedef long int	  count_int;

typedef	unsigned char	char_type;

#define BLOCK_MASK	0x80

#define INIT_BITS 9			/* initial number of bits/code */




int n_bits;				/* number of bits/code */
int maxbits = BITS;			/* user settable max # bits/code */
code_int maxcode;			/* maximum code, given n_bits */
code_int maxmaxcode = 1 << BITS;	/* should NEVER generate this code */
# define MAXCODE(n_bits)	((1 << (n_bits)) - 1)

count_int htab [HSIZE];
unsigned short codetab [HSIZE];
#define htabof(i)	htab[i]
#define codetabof(i)	codetab[i]


extern FILE *vmcorefd;

#define tab_prefixof(i)	codetabof(i)
# define tab_suffixof(i)	((char_type *)(htab))[i]
# define de_stack		((char_type *)&tab_suffixof(1<<BITS))

code_int free_ent = 0;			/* first unused entry */

code_int getcode();


struct dump_info  header;
int *comp_map;
/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
int block_compress = BLOCK_MASK;
int clear_flg = 0;

/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */ 
#define FIRST	257	/* first free entry */
#define	CLEAR	256	/* table clear output code */


/*read header and compression map*/

readhdr()
{

	if(maxbits < INIT_BITS) maxbits = INIT_BITS;
	if (maxbits > BITS) maxbits = BITS;
	maxmaxcode = 1 << maxbits;

      	if (fread(&header, sizeof(header),1,vmcorefd) <= 0) {
		perror("hdr read");
		exit(1);
	}
	if (header.dump_magic != NEW_MAGIC || header.revision != VER1)
		return(-1);
	comp_map = (int *)(malloc(header.comp_size));
	if (fread(comp_map, 1 , header.comp_size,vmcorefd) <= 0) {
		perror("hdr read");
		exit(1);
	}
	return(header.comp_size);
}


char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
static unsigned char seg_done;
static unsigned so_far;
static unsigned  cur_seg;
static unsigned  cur_seg_size;
static char *mem_addr;
/*
 * Decompress stdin to stdout.  This routine adapts to the codes in the
 * file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.  The tables used herein are shared
 * with those of the compress() routine.  See the definitions above.
 */

decompress(seg,address) 
	int seg;
	char *address;
{
	register char_type *stackp;
	register int finchar;
	register code_int code, oldcode, incode;
	unsigned next_seg;
	int size = 0;

	mem_addr = address;
	cur_seg = seg;
	seg_done = 0;
	so_far = 0;
	cur_seg_size = 0;

    	/*
     	* As above, initialize the first 256 entries in the table.
     	*/

	maxcode = MAXCODE(n_bits = INIT_BITS);
	for ( code = 255; code >= 0; code-- ) {
		tab_prefixof(code) = 0;
		tab_suffixof(code) = (char_type)code;
	}
	free_ent = ((block_compress) ? FIRST : 256 );
#ifdef DEBUG
	fprintf(stderr,"current seg = %d pos = %d\n",cur_seg,comp_map[cur_seg]);
#endif DEBUG
	if (comp_map[cur_seg]  < 0){
		return(-1);
	}
	fseek(vmcorefd,comp_map[cur_seg],0);
	next_seg = cur_seg;
	while (TRUE) {
		++next_seg;
		if (comp_map[next_seg] > 0){
			cur_seg_size = comp_map[next_seg] 
				- comp_map[cur_seg];
			break;
		}
		if (comp_map[next_seg] == SEG_MISSING)
			continue;
		if (comp_map[next_seg] == EOF_MAP){
			cur_seg_size = header.data_end; 
				- comp_map[cur_seg];
			break;
		}	
	}	
	if (cur_seg_size == header.seg_size) {

		if ((size = fread(mem_addr,1,header.seg_size,vmcorefd)) == 0) {
			perror(stderr,"fread");
			return(-1);
		}
		return(size);
	}

	finchar = oldcode = getcode();
	if(oldcode == -1){	/* EOF already? */
		return -1;			/* Get out of here */
	}

	*mem_addr++ = (char)finchar;

	stackp = de_stack;

	while ( (code = getcode()) > -1 ) {

		if ( (code == CLEAR) && block_compress ) {
	    		for ( code = 255; code >= 0; code-- )
				tab_prefixof(code) = 0;
	    	clear_flg = 1;
	    	free_ent = FIRST - 1;
	    	if ( (code = getcode ()) == -1 )	/* O, untimely death! */
			break;
		}
		incode = code;
		/*
	 	* Special case for KwKwK string.
	 	*/
		if ( code >= free_ent ) {
	           	*stackp++ = finchar;
	    	code = oldcode;
		}

		/*
	 	* Generate output characters in reverse order
	 	*/
		while ( code >= 256 ) {
	    		*stackp++ = tab_suffixof(code);
	    		code = tab_prefixof(code);
		}
		*stackp++ = finchar = tab_suffixof(code);

		/*
	 	* And put them out in forward order
	 	*/
		do
			*mem_addr++ = *--stackp;
		while ( stackp > de_stack );

		/*
	 	* Generate the new entry.
	 	*/
		if ( (code=free_ent) < maxmaxcode ) {
	    		tab_prefixof(code) = (unsigned short)oldcode;
	    		tab_suffixof(code) = finchar;
	    		free_ent = code+1;
		} 
		/*
	 	* Remember previous code.
	 	*/
		oldcode = incode;
	}
	return(mem_addr - address);
}


code_int
getcode() 
{
	register code_int code;
	static int offset = 0, size = 0;
	static char_type buf[BITS];
	register int r_off, bits;
	register char_type *bp = buf;
	register unsigned amount;
    
    	if ( clear_flg > 0 || offset >= size || free_ent > maxcode ) {
	/*
	 * If the next entry will be too big for the current code
	 * size, then we must increase the size.  This implies reading
	 * a new buffer full, too.
	 */
		if ( free_ent > maxcode ) {
	    		n_bits++;
	    		if ( n_bits == maxbits )
			maxcode = maxmaxcode;	/* won't get any bigger now */
	    	else
			maxcode = MAXCODE(n_bits);
		}
		if ( clear_flg > 0) {
    	    		maxcode = MAXCODE (n_bits = INIT_BITS);
	    		clear_flg = 0;
		}
		if (seg_done) 
			return -1;
		if ((so_far + n_bits) >=  cur_seg_size){
			amount = cur_seg_size - so_far;
			seg_done = 1;
        	}else
			amount = n_bits;

		size = fread( buf, 1, amount, vmcorefd );

		if ( size <= 0 ){
	    		return -1;			/* end of file */
		}
		so_far += size;
		offset = 0;
		/* Round size down to integral number of codes */
		size = (size << 3) - (n_bits - 1);
	}
	r_off = offset;
    	bits = n_bits;
	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;
	/* Get first part (low order bits) */
	code = (*bp++ >> r_off);
	bits -= (8 - r_off);
	r_off = 8 - r_off;		/* now, offset into code word */
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
	    code |= *bp++ << r_off;
	    r_off += 8;
	    bits -= 8;
	}
	/* high order bits. */
	code |= (*bp & rmask[bits]) << r_off;
    	offset += n_bits;

    	return code;
}
