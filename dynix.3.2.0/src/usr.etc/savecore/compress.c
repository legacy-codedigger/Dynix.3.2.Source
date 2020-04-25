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

#ident "$Header: compress.c 1.3 1991/08/29 00:15:18 $"


/*
 * Ccompress - crash_compress program
 */

#define	min(a,b)	((a>b) ? b : a)

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <setjmp.h>
#ifdef _SEQUENT_
#include <fcntl.h>
#include <unistd.h>
#endif
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

typedef long int	count_int;

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

code_int hsize = HSIZE;			/* for dynamic table sizing */


#define tab_prefixof(i)	codetabof(i)
# define tab_suffixof(i)	((char_type *)(htab))[i]
# define de_stack		((char_type *)&tab_suffixof(1<<BITS))

code_int free_ent = 0;			/* first unused entry */



/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
int block_compress =BLOCK_MASK;
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


static int offset;
long int in_count = 1;			/* length of input */
long int bytes_out;			/* length of compressed output */
static unsigned char *outbuf;
static buf_ptr;
struct dump_info header;
int *comp_map;
char *inbuf;
unsigned num_ent;
unsigned seg_size;
extern errno;
extern jmp_buf no_compress;

char *valloc();

init_compress(dump_size,unit_sz,startp)
	int dump_size;
	unsigned unit_sz;
	int *startp;
{
#ifdef _SEQUENT_
	setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
#else
	setlinebuf( stderr );
#endif

	seg_size = unit_sz;
        if (maxbits < INIT_BITS) maxbits = INIT_BITS;
        if (maxbits > BITS) maxbits = BITS;
        maxmaxcode = 1 << maxbits;
        /*
         * tune hash table size for small files -- ad hoc,
         * but the sizes match earlier #defines, which
         * serve as upper bounds on the number of output codes.
         */
	hsize = HSIZE;
	if (seg_size < (1 << 12))
		hsize = min (5003, HSIZE);
	else if (seg_size < (1 << 13))
		hsize = min (9001, HSIZE);
	else if (seg_size < (1 << 14))
		hsize = min (18013, HSIZE);
	else if (seg_size < (1 << 15))
		hsize = min (35023, HSIZE);
	else if (seg_size < 47000) 
		hsize = min (50021, HSIZE);

	num_ent = dump_size / seg_size;

	if (dump_size % seg_size)
		num_ent++;

	/*last entry in the map will be -1*/
	num_ent++;                          


	header.dump_magic = NEW_MAGIC;
	header.revision = VER1;
	header.seg_size = seg_size;
	header.comp_size = num_ent * (sizeof(int)/sizeof(char));


	if ((inbuf = valloc(seg_size)) == 0){
		printf("init_compress:valloc failed\n");
		exit(1);
	}
    	if ((comp_map = (int *)(malloc(num_ent * sizeof(int)))) == 0) {
		printf("init_compress:malloc failed\n");
		exit(1);
	}
	*startp = sizeof(header) + header.comp_size;
	return(num_ent-1);
}

write_hdr(ofd)
	int ofd;
{

        if ((int)(header.data_end = lseek(ofd, 0, 1)) == -1){
                perror("write_hdr:seek failed");
                exit(1);
        }

    	if (lseek(ofd, 0, 0) == -1) {
		perror("write_hdr:seek failed");
		exit(1);
	}
    	if ((write(ofd, (char *) &header, sizeof(struct dump_info))) == -1) {
    		perror("write_hdr:write failed");
    		exit(1);
  	}
    	if ((write(ofd, (char *) comp_map, header.comp_size)) == -1) {
    		perror("write_hdr:write failed");
    		exit(1);
    	}
}
#define QUANTUM (10*1024*1024)

static char buf[BITS];

char_type lmask[9] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

compress(infile,pos,outp)
	char *infile;
	int pos;
	unsigned char *outp;
{
	register long fcode;
    	register code_int i = 0;
    	register int c;
    	register code_int ent;
    	register int disp;
    	register code_int hsize_reg;
    	register int hshift;
	register unsigned char *pointer;

	int ifd;
   	 
#ifdef DEBUG
	printf("pos = %d seg_size = %d outp = %x\n",pos,seg_size,outp);
#endif /* DEBUG */
	offset = 0;
	bytes_out = 0;
	clear_flg = 0;
	ratio = 0; 
	in_count = 1;
	errno =0;
	buf_ptr = 0;
	outbuf = outp;
	checkpoint = CHECK_GAP;
	maxcode = MAXCODE(n_bits = INIT_BITS);
	if ((ifd = open(infile,O_RDONLY)) == -1){
		perror("compress:can't open input file");
		return(-1);
	}

        if (lseek(ifd, pos, 0) == -1){
		perror("compress:seek into input file failed");
		return(-1);
	}
        if (read(ifd, inbuf, seg_size) == -1){
		perror("compress:read failed");
		return(-1);
	}
	pointer = (unsigned char *)inbuf;


	free_ent = ((block_compress) ? FIRST : 256);
	ent = *pointer++;
	hshift = 0;
	for (fcode = (long) hsize;  fcode < 65536L; fcode *= 2L)
		hshift++;
	hshift = 8 - hshift;		 /*set hash code range bound */

	hsize_reg = hsize;
	cl_hash ((count_int) hsize_reg);

	for(in_count=1;in_count < seg_size;in_count++){

		c = *pointer++;
		fcode = (long) (((long) c << BITS) + ent);
		i = ((c << hshift) ^ ent);	/* xor hashing */

		if (htabof (i) == fcode) {
			ent = codetabof (i);
			continue;
		} else if ((long)htabof (i) < 0)	/* empty slot */
			goto nomatch;
		disp = hsize_reg - i;	/* secondary hash (after G. Knott) */
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
		output(ifd,(code_int) ent);
		ent = c;
		if (free_ent < maxmaxcode) {
			codetabof (i) = free_ent++; 	/* code -> hashtable */
			htabof (i) = fcode;
		}
		else if ((count_int)in_count >= checkpoint 
				&& block_compress) {
			cl_block(ifd);
		}
	}
	/*
	 * Put out the final code.
	 */
	output(ifd,(code_int)ent);
	output(ifd,(code_int)-1);
	close(ifd);
	return(buf_ptr);
}

output(ifd,code)
	int ifd;
	code_int  code;
{

	register int r_off = offset, bits= n_bits;
	register char *bp = buf;

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
		if(bits)
	    		*bp = code;
		offset += n_bits;
		if (offset == (n_bits << 3)) {
	    		bp = buf;
	    		bits = n_bits;
	    		bytes_out += bits;

	    		do {
				write_out(ifd,bp,1);
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
	     	 	 * Write the whole buffer, because the input side 
	     	 	 * won't discover the size increase until after it 
			 * has read it.
	     	 	 */
			if (offset > 0) {
				write_out(ifd,buf,n_bits);
				bytes_out += n_bits;
	    		}
	    		offset = 0;

	    		if (clear_flg) {
	        		maxcode = MAXCODE (n_bits = INIT_BITS);
	        		clear_flg = 0;
	    		} else {
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
	    		write_out(ifd,buf,(offset + 7) / 8);
		bytes_out += (offset + 7) / 8;
		offset = 0;
	}
}


copystat(ifname, ofname,ofd)
	char *ifname, *ofname;
	int ofd;
{
   	struct stat statbuf;
   	int mode;
   	time_t timep[2];

   	close(ofd);
   	if (stat(ifname, &statbuf)) {		/* Get stat on input file */
		fprintf(stderr, "copystat:");
		perror(ifname);
		return;
   	}
   	if ((statbuf.st_mode & S_IFMT/*0170000*/) != S_IFREG/*0100000*/) {
		fprintf(stderr, " -- not a regular file: unchanged");
   	} else if (statbuf.st_nlink > 1) {
		fprintf(stderr, "%s: ", ifname);
		fprintf(stderr, " -- has %d other links: unchanged",
		statbuf.st_nlink - 1);
   	} else {		/* ***** Successful Compression ***** */
		mode = statbuf.st_mode & 07777;
		if (chmod(ofname, mode))		/* Copy modes */
	    		perror(ofname);
		/* Copy ownership */
		chown(ofname, statbuf.st_uid, statbuf.st_gid);

		/* Update last accessed and modified times*/
		timep[0] = statbuf.st_atime;
		timep[1] = statbuf.st_mtime;
		utime(ofname, timep);

		return;		/* Successful return */
   	}

   	/* Unsuccessful return -- one of the tests failed */
       if (unlink(ofname))
	        perror(ofname);

}


cl_block (ifd)		/* table clear for block compress */
	int ifd;
{
   	register long int rat;

   	checkpoint = in_count + CHECK_GAP;

   	if(in_count > 0x007fffff) {	/* shift will overflow */
		rat = bytes_out >> 8;
		if(rat == 0) {		/* Don't divide by zero */
	    		rat = 0x7fffffff;
		} else {
	    		rat = in_count / rat;
		}
   	} else {
		rat = (in_count << 8) / bytes_out;	/* 8 fractional bits */
   	}
   	if ( rat > ratio ) {
		ratio = rat;
   	} else {
		ratio = 0;
		cl_hash ((count_int) hsize);
		free_ent = FIRST;
		clear_flg = 1;
		output (ifd, (code_int) CLEAR);
   	}
}

cl_hash(hsize)		/* reset code table */
	register count_int hsize;
{
	register count_int *htab_p = htab+hsize;
	register long i;
	register long m1 = -1;

	i = hsize - 16;
	do {				/* might use Sys V memset(3) here */
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
   	for (i += 16; i > 0; i--) {
		*--htab_p = m1;
	}
}

write_out(ifd,buf,size)
	int ifd;
	char *buf;
	int size;
{
	while (size-- > 0) {
		if (buf_ptr < (seg_size - 1)) {
			outbuf[buf_ptr++] = *buf++;
		} else {
			close(ifd);
			longjmp(no_compress, 1);
		}
	}
}
