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

#ifdef RCS
static char rcsid[] = "$Header: compress.c 1.1 1991/04/23 20:56:15 $";
#endif

/* 
 * Compress -  compression code
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <machine/cfg.h>
#include "saio.h"
#include "sec.h"
#include "dump.h"


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
#define BLOCK_MASK	0x80
#define INIT_BITS 9		/* initial number of bits/code */
#define K1             (1024)
#define K32		(32 * K1)

#define	min(a,b)	((a>b) ? b : a)
#define MAXCODE(n_bits)	((1 << (n_bits)) - 1)
#define IS_TAPE(f)      (devsw[iob[(f)-3].i_ino.i_dev].dv_flags == D_TAPE)

typedef long int	code_int;
typedef long int	count_int;
typedef	unsigned char	char_type;


int n_bits;				/* number of bits/code */
int maxbits = BITS;			/* user settable max # bits/code */
code_int maxcode;			/* maximum code, given n_bits */
code_int maxmaxcode = 1 << BITS;	/* should NEVER generate this code */

count_int htab [HSIZE];
unsigned short codetab [HSIZE];
#define htabof(i)	htab[i]
#define codetabof(i)	codetab[i]

code_int hsize = HSIZE;			/* for dynamic table sizing */


#define tab_prefixof(i)	codetabof(i)
# define tab_suffixof(i)	((char_type *)(htab))[i]
# define de_stack		((char_type *)&tab_suffixof(1<<BITS))

code_int free_ent = 0;			/* first unused entry */



/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
int block_compress = BLOCK_MASK;
int clear_flg = 0;
long int ratio = 0;
#define CHECK_GAP 10000	/* ratio check interval */
count_int checkpoint = CHECK_GAP;
/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */ 
#define FIRST	257	/* first free entry */
#define	CLEAR	256	/* table clear output code */

long int in_count = 1;			/*length of input */
long int bytes_out;			/*length of compressed output */

off_t data_start;                       /*compressed data starts here*/
unsigned file_offset;                   /*current offset on the swap device*/ 
int *comp_map;                          /*compression map: keeps a record of
				          where each compreesed seg. starts*/	
unsigned ent_no;                        /*current segment being processed*/
static int offset;
unsigned num_ent;                       /* num. of entries in comp. map*/
unsigned compressed_size;               /*size of compressed data for cur seg*/
unsigned total_size;                    /*size of compressed data*/
extern int errno;
static char *outbuf;                    /*output buffer 32k big*/
static char *seg_buf;			/*output buffer UNIT_SZ big*/
static buf_ptr;                         /*next free index into outbuf*/
static seg_buf_ptr;                     /*next free index into seg_buf*/
char *num_write_p;                      /* set when outbuf is flushed*/
int dmesg_end;

/* Set up the data structures needed for compression.  */

init_compress(fd,header,dump_size,mem_ptr,num_p,tape)
	int fd;
	struct dump_info *header;
	int dump_size;
	int mem_ptr;
	char *num_p;
	char tape;
{
	char  *calloc();


	if (maxbits < INIT_BITS) maxbits = INIT_BITS;
	if (maxbits > BITS) maxbits = BITS;
	maxmaxcode = 1 << maxbits;
	
	/*
	 * tune hash table size for small files -- ad hoc,
	 * but the sizes match earlier #defines, which
	 * serve as upper bounds on the number of output codes. 
	 */
	hsize = HSIZE;
	if (UNIT_SZ < (1 << 12))
	    hsize = min (5003, HSIZE);
	else if (UNIT_SZ < (1 << 13))
	    hsize = min (9001, HSIZE);
	else if (UNIT_SZ < (1 << 14))
	    hsize = min (18013, HSIZE);
	else if (UNIT_SZ < (1 << 15))
	    hsize = min (35023, HSIZE);
	else if (UNIT_SZ < 47000)
	    hsize = min (50021, HSIZE);

        num_ent = dump_size / UNIT_SZ;

        if (dump_size % UNIT_SZ)
		num_ent++;
        num_ent++;                          /*last entry in the map will be -1*/


	/* initialize header with size of compression map */
	header->comp_size = num_ent * (sizeof(int)/sizeof(char));

	/* allocate the compression map */
	comp_map = (int *)(calloc(num_ent * sizeof(int)));

	/* calculate initial number of segments not dumped. Mark them as
	 * missing 
	 */
	num_ent = mem_ptr / UNIT_SZ;
	while (num_ent--)
		comp_map[ent_no++] = SEG_MISSING;


	/* data starts after the header and the compression map. Round to
	 * a 512 byte boundary due to alignment requirements of the 
	 * standalone write.
	 */
	data_start = roundup((sizeof(struct dump_info) + header->comp_size),
			DEV_BSIZE);

	num_write_p = num_p;
	/* Allocate the 32 Kb output buffer */
	callocrnd(RAWALIGN);
	outbuf =  calloc(K32);

	/* Allocate the outbuf buffer which is reused for each segment */
	callocrnd(RAWALIGN);
	seg_buf =  calloc(UNIT_SZ);

	total_size = data_start;
	file_offset = data_start;

	/* Is data_start a integral multiple of 32K */
	*num_write_p = file_offset/K32;

	/* if tape device, space forward. The disk seeks are done later */
	if (tape){
		write_out(fd,header,sizeof(struct dump_info));
		write_buf(fd,*num_write_p);
	}

	/* Bump the buf_ptr to account for the rest of the header space. */
	buf_ptr = file_offset % K32;
}

/* Write out dump_info and the compression map */
write_hdr(fd,dev,offset,header,tape_flag)
	int	fd;
	char	*dev;
	int	offset;
	struct	dump_info *header;
	char	tape_flag;
{
	register int p;
	int 	sz;
	char 	*b_ptr, *h_ptr, *c_ptr;
	char 	*buffer;	
	int head_sz = sizeof(struct dump_info);
	int comp_size = header->comp_size;


#ifdef DEBUG
	printf("**writing header\n**");	
#endif DEBUG

	/* initialize remaining header fields */

	/* round up for savecore's benefit */
	header->compressed_size = roundup(total_size,K32);

	/* data ends here, and dmesg starts here */
	header->data_end = file_offset;

	/* dmesg ends here */
	header->dmesg_end = dmesg_end;

	/* End of Map indicator for the compression map */
	comp_map[ent_no] = EOF_MAP;
	errno = 0;

	if (!tape_flag) {
		if ((fd = open(dev, 2)) == -1){
			printf("Can't open dump device for writing header\n");
			return(-1);
			}
		if (lseek(fd,offset,0) == -1) {
			printf("seek for writing header failed");
			return(-1);
		}
	}

	callocrnd(RAWALIGN);
	if ((buffer = calloc(head_sz+comp_size)) == 0){
		printf("header write failed\n");
		return(-1);
	}
	b_ptr = (char *) buffer;
	h_ptr = (char *) header;
	c_ptr = (char *) comp_map;
	p = head_sz;
	while(p--)
		*b_ptr++ = *h_ptr++;
	while(comp_size--)
		*b_ptr++ = *c_ptr++;

	sz = write(fd, buffer ,(head_sz + header->comp_size)); 
	if (sz == -1){
		printf("header write failed\n");
		printf("errno = %d\n",errno);
		return(-1);
	}
	close(fd);
}

static char buf[BITS];

char_type lmask[9] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

compress(fdesc,pointer)
	int fdesc;
	unsigned char *pointer;
{
	register long fcode;
	register code_int i = 0;
	register int c;
	register code_int ent;
	register int disp;
	register code_int hsize_reg;
	register int hshift;
   	unsigned char *mem_loc = pointer; 

	/* initialize other static variables */

	offset = 0;
	bytes_out = 0;
	clear_flg = 0;
	ratio = 0;
	in_count = 1;
	compressed_size = 0;
	seg_buf_ptr = 0;

	checkpoint = CHECK_GAP;
	maxcode = MAXCODE(n_bits = INIT_BITS);
	free_ent = ((block_compress) ? FIRST : 256);
	ent = *pointer++;

	/* store the location of the compressed data for this segment */
	comp_map[ent_no++] = file_offset;

	hshift = 0;
	for (fcode = (long) hsize; fcode < 65536L; fcode *= 2L)
		hshift++;
	hshift = 8 - hshift;		/* set hash code range bound */

	hsize_reg = hsize;
	cl_hash( (count_int) hsize_reg);		/* clear hash table */

	/* process each char in the UNIT_SZ big segment */
	for(in_count=1;in_count < UNIT_SZ;in_count++) {

		c = *pointer++;
		fcode = (long) (((long) c << maxbits) + ent);
		i = ((c << hshift) ^ ent);	/* xor hashing */
	
		if (htabof (i) == fcode) {
			ent = codetabof (i);
			continue;
		} else if ((long)htabof (i) < 0)	/* empty slot */
			goto nomatch;

		/* secondary hash (after G. Knott) */
		disp = hsize_reg - i;		
		if (i == 0)
			disp = 1;
	probe:
		if ((i -= disp) < 0)
			i += hsize_reg;
	
		if (htabof (i) == fcode) {
			ent = codetabof (i);
			continue;
		}
		if ((long)htabof (i) > 0) 
			goto probe;
	nomatch:

		if (output ((code_int) ent) == -1)
			return(-1);
		ent = c;
		if (free_ent < maxmaxcode) {
			codetabof (i) = free_ent++;	/* code -> hashtable */
			htabof (i) = fcode;
		}
		else if ((count_int)in_count >= checkpoint && block_compress) {
			cl_block();
		}  
	}

	/*
	 * Put out the final code.
	 */

	if (output((code_int)ent) == -1)
		return(-1);
	if (output((code_int)-1) == -1)
		return(-1);

	if (compressed_size >= UNIT_SZ) {
		/* Size of compressed data is >= then input. Leave this
		 * segment uncompressed in the dump. Set the compressed
		 * size to UNIT_SZ which uncompress understands to mean
		 * that the data is not compressed 
		 */
		compressed_size = UNIT_SZ;
		for (i=0;i < UNIT_SZ; i++)
			seg_buf[i] = *mem_loc++;
	}

	/* Transfer the contents of this buffer into outbuf */
	write_out(fdesc,seg_buf,compressed_size);
	total_size += compressed_size;
	return(0);
}

output(code)
	code_int  code;
{

	register int r_off = offset, bits= n_bits;
	register char * bp = buf;

	if (code >= 0) {

		/*
	 	 * Get to the first byte.
	 	 */
		bp += (r_off >> 3);
		r_off &= 7;

		/*
	 	 * Since code is always >= 8 bits, only need to mask the first
	 	 * hunk on the left.
	 	 */
		*bp = (*bp & rmask[r_off]) | (code << r_off) & lmask[r_off];
		bp++;
		bits -= (8 - r_off);
		code >>= 8 - r_off;

		/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
		if (bits >= 8) {
	    		*bp++ = code;
	    		code >>= 8;
	    		bits -= 8;
		}
		/* Last bits. */
		if (bits)
	    		*bp = code;
		offset += n_bits;
		if (offset == (n_bits << 3)) {
	    		bp = buf;
	    		bits = n_bits;
	    		bytes_out += bits;

	    		do {
				if (buffer_data(bp,1) == -1)
					return(-1);
				bp++;
	    		} while(--bits);
	    		offset = 0;
		}

		/*
	 	 * If the next entry is going to be too big for the code size,
	 	 * then increase it, if possible.
	 	 */
		if (free_ent > maxcode || (clear_flg > 0)) {

	    	/*
	     	 * Write the whole buffer, because the input side won't
	     	 * discover the size increase until after it has read it.
		 */
	    		if (offset > 0) {
				if(buffer_data(buf, n_bits) == -1)
					return(-1);	
				bytes_out += n_bits;
	    		}
	    		offset = 0;

	    		if (clear_flg) {
    	        		maxcode = MAXCODE (n_bits = INIT_BITS);
	        		clear_flg = 0;
	    		}
	    		else {
	    			n_bits++;
	    			if (n_bits == maxbits)
		    			maxcode = maxmaxcode;
	    			else
		    			maxcode = MAXCODE(n_bits);
	    		}
		}
	} else {

		/*
	 	 * At EOF, write the rest of the buffer.
	 	 */
		if (offset > 0)
			if (buffer_data(buf, (offset + 7) / 8) == -1)
				return(-1);
		bytes_out += (offset + 7) / 8;
		offset = 0;
	}
	return(0);
}

cl_block()		/* table clear for block compress */
{
	register long int rat;

	checkpoint = in_count + CHECK_GAP;

	if (in_count > 0x007fffff) {	/* shift will overflow */
		rat = bytes_out >> 8;
		if (rat == 0) {		/* Don't divide by zero */
	    		rat = 0x7fffffff;
		} else {
	    		rat = in_count / rat;
		}
    	} else {
		rat = (in_count << 8) / bytes_out;	/* 8 fractional bits */
    	}
	if (rat > ratio) {
		ratio = rat;
    	} else {
		ratio = 0;
 		cl_hash ((count_int) hsize);
		free_ent = FIRST;
		clear_flg = 1;
		if (output ((code_int)CLEAR) == -1)
			return(-1);
		
    	}
}

cl_hash(hsize)		/* reset code table */
	register count_int hsize;
{
	register count_int *htab_p = htab+hsize;
	register long i;
	register long m1 = -1;

	i = hsize - 16;
 	do {				
		*(htab_p-16) = m1;
		*(htab_p-15) = m1;
		*(htab_p-14) = m1;
		*(htab_p-13) = m1;
		*(htab_p-12) = m1;
		*(htab_p-11) = m1;
		*(htab_p-10) = m1;
		*(htab_p-9) = m1;
		*(htab_p-8) = m1;
		*(htab_p-7) = m1;
		*(htab_p-6) = m1;
		*(htab_p-5) = m1;
		*(htab_p-4) = m1;
		*(htab_p-3) = m1;
		*(htab_p-2) = m1;
		*(htab_p-1) = m1;
		htab_p -= 16;
	} while ((i -= 16) >= 0);
    	for (i += 16; i > 0; i--)
		*--htab_p = m1;
}

/* put compressed data in the seg_buf until compressed size is >= UNIT_SZ */
buffer_data(buf,size)
	char *buf;
	int size;
{
	compressed_size += size;
	if (compressed_size >= UNIT_SZ) { 
		return;
	}
	while (size-- > 0) {
		seg_buf[seg_buf_ptr++] = *buf++;
	}	
}

/* tranfer buf to outbuf. out_buf gets written out when it fills up */
write_out(fdesc, buf, size)
	int fdesc;
	char *buf;
	int size;
{
	file_offset += size;
	while (size-- > 0){
		if (buf_ptr < K32)
			outbuf[buf_ptr++] = *buf++;
		else {
#ifdef DEBUG
			printf("***flushing buffer***\n");
#endif DEBUG
			if (write(fdesc,outbuf,K32) != K32) {
				printf("write_error \n");
				return(-1);
			}
			(*num_write_p)++;
			buf_ptr = 0;
			outbuf[buf_ptr++] = *buf++;
		}
	}
	return(0);
}

/* Write out outbuf */
flush_buf(fdesc)
	int fdesc;
{
	int sz;
	errno = 0;
	if (buf_ptr){
#ifdef DEBUG
		printf("***flushing buffer***\n");
#endif DEBUG
		sz  = roundup(buf_ptr,512);
		if (write(fdesc,outbuf,sz) == -1) {
			printf("flush error \n");
			printf("errno = %d\n",errno);
			return(-1);
		}	
		return(sz);
	}
	return(0);
}

/* A variation of flush_buf used to simulate a lseek, when leaving space 
 * for the compression header.
 */
write_buf(fd,num)
	int fd;
	int num;
{
	while (num--) {
		if (write(fd,outbuf,K32) == -1) {
			printf("error in writing outbuf\n");
			printf("errno = %d\n",errno);
			return(-1);
		}	
	}
	return(0);
}

/* Mark the segments corresponding to a hole, as missing. */
update_map()
{
	register unsigned i;
	for(i=0; i < MC_CLICK/UNIT_SZ; i++)
		comp_map[ent_no++] = SEG_MISSING;
}


/* To conserve memory reuse outbuf to store the straightened out dmesg buffer */

write_dmesg(fd)
	int fd;
{
	int size;
	errno = 0;

	dmesg_end  = roundup(total_size,512);

	if ((size = smgetlog(outbuf)) == -1)
		return(0);
#ifdef DEBUG
	printf("smgetlog returns %d bytes \n",size);
#endif DEBUG
	size  = roundup(size,512);
	if (write(fd,outbuf,size) == -1){
		printf("dmseg write failed\n");
		printf("errno = %d\n",errno);
		return(-1);
	}
	dmesg_end += size;
	total_size += size;
#ifdef DEBUG
	printf("dmesg written\n");
#endif DEBUG
}
