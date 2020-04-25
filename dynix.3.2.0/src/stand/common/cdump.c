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
static char rcsid[] = "$Header: cdump.c 1.1 1991/04/23 20:50:47 $";
#endif

/*
 * cdump -- standalone compressed memory dumper
 *
 * usage:
 *	b <flags> <dumpername> <dumpdev> <offset> <unixname> [<size>] [-o]
 *	b <flags> <dumpername> -f <dumplistfile> <unixfile> [-o]
 *
 * example:
 *	b 80 sd(48,0)stand/cdump sd(48,1) 1000 /dev/sd0b
 *	b 80 sd(48,0)stand/cdump -f sd(48,0)/etc/DUMPLIST /etc/DUMPLIST
 *
 * this program copies memory to <offset> blocks past the beginning
 * of <dumpdev>.  <unixname> should mean the same device to unix as
 * <dumpdev> does to the standalone (for the convenience of savecore).
 * -f allows multiple dump devices to be specifed.  Each line in <unixfile>
 * is of the format:
 * 	dumpdev1 offset1 unixname1 [size1]
 * 	dumpdev2 offset2 unixname2 [size2]
 *
 * <size> is used to override the the amount of memory to dump.
 * The size, specified in blocks, is limited to
 * (size of the partition) - <offset>) and is set to be the min
 * of this limit and the size of memory read from the
 * config structure.  <size> will not be allowed to be greater than
 * the amount of memory configured or the size of the partition.
 * -o forces an overwrite of an existing dump 
 *
 * turns off the RB_DUMP flag and sets the config structure address to
 * zero in the firmware before it exits.
 *
 *
 * notes:
 *	
 *	the dump string ("b 80 sd(48,0)stand/cdump...") should be stored
 *	permanently via the pup firmware wn1 command.  then "b 80" will
 *	be sufficient.
 *
 *	for dumping the kernel, the config structure should be placed
 *	before the end of kernel text space by the firmware. 
 */

#include <sys/file.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "saio.h"
#include <machine/pte.h>
#include <machine/mftpr.h>
#include <sys/reboot.h>
#include <machine/cfg.h>
#include <machine/slicreg.h>
#include "sec.h"
#include "dump.h"

#define	MIN(a,b) (((a)<(b))?(a):(b))

#define K1		1024
#define K32		(32 * K1)
#define M1		(K1 * K1)

#define OPTARGS		2		/* Number of optional arguments */
#define TRUE		1
#define FALSE		0
#define DUMP_DEV_SIZE   16 * M1         /* 16MB which is currently the   */
					/* smallest swap partition size. */
#define	IS_TAPE(f)	(devsw[iob[(f)-3].i_ino.i_dev].dv_flags == D_TAPE)
#define	IS_DIGIT(d)	((d) >= '0' && (d) <= '9')

struct dump_info dump_header  = {	/* dump magic number and size */
	NEW_MAGIC,
	-1,
	-1,
	VER1,
	UNIT_SZ,
	-1,
	-1,
	-1,
};

int	dump_size = -1;
int	disk_space = -1;		/* space avlbl. on device for a dump */
int	* magic;
caddr_t	calloc();
caddr_t	magic_buf;
int	psize;				/* size of the partition */
int	chunk;
char	* mem_pointer;
int	mem_left;
char 	istape;

main()
{
	char *device, *first_dev;	/* name of device to dump on */
	unsigned int offset;		/* offset in bytes from start of dev */
	unsigned int first_off;
	int overwrite = 0;		/* nonzero => dump even if one there */
	int fd, first_fd;

	printf("\nMemory dumper\n");

	/*
	 * the firmware clears memory if RB_DUMP isn't set.
	 */
	if ((CD_LOC->c_boot_flag & RB_DUMP) == 0)
		printf("Warning:  memory was probably zeroed!\n");

	/*
	 * look at the boot string to find out where to dump
	 */
	while (find_where(&device, &offset, &overwrite) == TRUE) {

		/*
		 * Already a dump there?
		 */
		++chunk;
		if ( already_a_dump(device, offset) == TRUE )
			if ( !overwrite )
				abort("Dump already in dump area");
			else
				printf("Overwriting previous dump.\n");

		/*
		 * Open dump device for writing
		 */
		fd = open(device, 2);
		if (fd < 0)
			abort("Can't open dump device for writing");

		/* start dumping at mem_pointer. mem_pointer points to 
		 * K32 after the value of brk. The initial memory has been
		 * trashed by cdump, thus no point in dumping it.
		 */
		if (chunk == 1) {
			mem_pointer = (char *)roundup(((int)calloc(0)+K32),UNIT_SZ);
		}
		/*
		 * Dump this chunk to the device
		 */

		set_dump_size(fd, offset);
		if (dump_size) {
			printf("\nDumping %d.%d MB to %s offset %d",
				dump_size / M1,
				(dump_size % M1) * 10 / M1,
				device, offset);
			/*
			 * Write dump header containing magic number and 
			 * dump size if this is the first chunk,
			 * and then start dumping memory.
			 */
			if (chunk == 1) {
				first_dev = device;
				dump_header.dump_size = CD_LOC->c_maxmem;
				first_off = offset;
			}
			if (dumpmem(fd, offset) == FALSE) {
				(void) close(fd);
				printf("\nDump failed.\n");
				return_to_firmware();
			}
		}
		if (!IS_TAPE(fd))	
			(void) close(fd);
		else
			istape = TRUE;
		dump_size = -1;
	}
	/* write out the compression header. This is written at the beginning
	 * of the file when dumping to disk, but after the end of data when
	 * dumpming to tape.
	 */
	if (write_hdr(fd,first_dev,first_off,&dump_header,istape) == -1){
		printf("\nDump failed.\n");
		return_to_firmware();
	}
	if (mem_left > 0)
		printf("\nWarning...%d bytes of memory not dumped!\n",
			mem_left);
	else
		printf("\nDump succeeded.\n");
	return_to_firmware();
}

extern char *next_word();

/*
 * parses the boot name to find the dump device and offset.  aborts
 * if it can't parse the line.  the dumper should be booted like:
 *
 *	b <flags> <dumpername> <dumpdev> <offset> <unixname> [<size>] [-o]
 *
 * the last four to six, (last 2 optional), fields turn up in the boot string.
 * or
 *
 *	b <flags> <dumpername> -f <dumplistfile> <unixfile> [-o]
 *
 * with the last argument optional.
 */

find_where(name_p, offset_p, overwrite_p)
	char **name_p;			/* name to fill in */
	unsigned int *offset_p;		/* offset to fill in */
	int *overwrite_p;		/* to overwrite a dump or not */
{
	register char *cp, *cpend;
	static int fflag = FALSE;
	static char buf[8192];
	static int size;
	static char *s_end;
	char *arg0, *arg1, *arg2, *arg3, *arg4, *arg5;
	int fd, i;

	if ( fflag == TRUE ) {
		cp = s_end + 1;
		if ((cp - buf) > size)
			return (FALSE);
		for (cpend = cp; cpend < &buf[size]; cpend++)
			if (*cpend == '\n')
				break;
		if (cpend >= &buf[size])
			return (FALSE);
		s_end = cpend;
		*cpend = '\0';
		arg1 = cp;
		arg2 = next_word( arg1, cpend );
		arg3 = next_word( arg2, cpend );
		arg4 = next_word( arg3, cpend );
		arg5 = NULL;
	} else {
		if ( chunk != 0 )
			return (FALSE);
		/*
		 * find the boot name in config table
		 */
		cp = CD_LOC->c_boot_name;
		i = CD_LOC->c_bname_size;
		cpend = &(CD_LOC->c_boot_name[i]);

		/*
		 * Crack line into arguments (up to 6 args).
		 */
		arg0 = cp;
		arg1 = next_word( arg0, cpend );
		arg2 = next_word( arg1, cpend );
		arg3 = next_word( arg2, cpend );
		arg4 = next_word( arg3, cpend );
		arg5 = next_word( arg4, cpend );

		/*
		 * get name of device to dump on into name_p.
		 */
		if (arg1 == NULL)
			abort("missing second argument");
		if (strcmp(arg1, "-f") == 0) {
			fflag = TRUE;
			if (arg2 == NULL)
				abort("No dump device file specified");
			fd = open(arg2, 0);
			if (fd < 0)
				abort("Can't open dump device file");
			size = read(fd, buf, sizeof(buf));
			(void) close(fd);
			if (size < 0)
				abort("Error reading dump device file");
			for (i=0; i < size; i++) {
				if (buf[i] == ' ' || buf[i] == '\t')
					buf[i] = '\0';
			}
			if (arg4 != NULL && strcmp(arg4, "-o") == 0)
				*overwrite_p = TRUE;
			cp = buf;
			for (cpend = buf; cpend < &buf[size]; cpend++)
				if (*cpend == '\n')
					break;
			if (cpend >= &buf[size])
				abort("Format error in dump device file");
			s_end = cpend;
			*cpend = '\0';
			arg1 = cp;
			arg2 = next_word( arg1, cpend );
			arg3 = next_word( arg2, cpend );
			arg4 = next_word( arg3, cpend );
			arg5 = NULL;
		}
	}

	*name_p = arg1;
	if ((fd = open(arg1, 0)) < 0)
		abort("Can't open dump device");	
	if (!IS_TAPE(fd)) {
		close(fd);
		psize = vtoc_getpsize(arg1) * DEV_BSIZE;
#ifdef VTOC_ONLY
		if (psize < 0) {
			printf("Bad partition or non-existent VTOC on device\n");
			printf("Add a VTOC to the disk using a formatter or use\n");
			printf("a legal partition number.\n\n");
			abort("Bad device %s\n", arg1);
		}
#endif
	} else {
		psize = -1;
		close(fd);
	}
	if (arg2 == NULL)
		abort("Missing offset");
	*offset_p = atoi(arg2) * DEV_BSIZE;
	if (arg3 == NULL)
		abort("Missing unix device name");
	if (arg4 != NULL)
		if ( IS_DIGIT(*arg4) ) {
			dump_size = atoi(arg4) * DEV_BSIZE;
			if (dump_size < 0) {
				dump_size = 0;
			}
		} else {
			dump_size = -1;
			if ( strcmp(arg4, "-o") == 0)
				*overwrite_p = TRUE;
		}
	if (arg5 != NULL)
		if ( strcmp(arg5, "-o") == 0)
			*overwrite_p = TRUE;
	close(fd);
	return (TRUE);
}

char *
next_word( p , e )
	char	*p;
	char	*e;
{

	if ( p == NULL || p >= e )
		return( NULL );
	while ( *p != '\0' ) {		/* skip letters */
		if ( ++p >= e )
			return( NULL );
	}
	while ( *p == '\0' ) {		/* skip nulls */
		if ( ++p >= e )
			return( NULL );
	}
	return( p );
}

/*
 * check dump location to see if there's a dump there already.
 */

already_a_dump(device, offset)
	char *device;
	unsigned int offset;
{
	int fd;

	/*
	 * Only check the first chunk for an existing dump
	 */
	if ( chunk != 1 )
		return ( FALSE );

	fd = open(device, 0);
	if (fd < 0)
		abort("Can't open dump device for reading");

	/*
	 * if it is tape, don't check for magic number
	 */
	if ( IS_TAPE(fd) ) {
		close(fd);
		return ( FALSE );
	}
	if (magic_buf == (caddr_t)NULL) {
		callocrnd(RAWALIGN);
		magic_buf = calloc(DEV_BSIZE);
	}
	lseek(fd, offset, 0);
	if (read(fd, (caddr_t)magic_buf, DEV_BSIZE) != DEV_BSIZE)
		abort("Can't read magic number");
	close(fd);
	magic = (int *)magic_buf;
	if ( *magic == DUMP_MAGIC || *magic == NEW_MAGIC)
		return ( TRUE );
	else
		return ( FALSE );
}

set_dump_size(fd, offset)
	int fd;
	unsigned int offset;
{
	int size;

	mem_left = CD_LOC->c_maxmem - (int)mem_pointer;
	/*
	 * Set the size of the dump based on whether or not an
	 * override size was specified and the amount of memory
	 * to dump.  If psize >= 0, then psize is the size of the
	 * VTOC partition that we are dumping to.  Otherwise, it's
	 * a non-VTOC partition. disk_space indicates the space available
	 * on a disk device for the compressed dump. The dumper keeps
	 * on a device until disk_space is exhausted.
	 */
	if (dump_size >= 0) {
		/*
		 * override size given.  Use the size as-is unless
		 * it goes off the end of a VTOC partition.
		 */
		size = MIN(mem_left, dump_size);
		if (psize >= 0) {
			size = MIN(size, psize - (int)offset);	
			disk_space = MIN(dump_size,psize - (int)offset);
		}
	} else if (psize >= 0) {
		/*
		 * no override size; VTOC disk
		 */
		size = MIN(mem_left, psize - (int)offset);
		disk_space = psize - (int)offset;
	} else  if (IS_TAPE(fd)) {
		/*
		 * no override size; dumping to tape
		 */
		size = mem_left;
	} else {
		/*
		 * no override size; non-VTOC disk
		 */
		size = MIN(mem_left, DUMP_DEV_SIZE - offset);
		disk_space = DUMP_DEV_SIZE - (int)offset;
	}
	if (size < 0)
		size = 0;
	dump_size = size;

	dump_size &= ~(K32 - 1);	/* K32  granularity */
	disk_space &= ~(K32 - 1);	/* K32  granularity */
}

static char num_writes;		/* number of 32K writes done to the device */

dumpmem(fd, offset)
	int fd;
	unsigned int offset;
{
	register int i;
	register int count, m;
	char *p;
	int amount=0;
	char istape = FALSE;

	num_writes = 0;
	if (IS_TAPE(fd))
		istape = TRUE;

	
	/* Setup for compression. */
	if (chunk == 1)
		init_compress(fd,&dump_header,CD_LOC->c_maxmem,(int)mem_pointer,			&num_writes,istape);

        /*
         * If it's not a tape, seek to offset. Also leave space for
	 * the compression header if it is bigger than 32k.
         */
        if (!istape)
		lseek(fd,offset+(num_writes*K32),0);

	if (!istape){
		/* count is the amount of space avlbl. on this device
		 * for dumping.
		 */
		count = disk_space - num_writes * K32;
		num_writes = 0;
	}else
		count = dump_size;

	p = mem_pointer;
	m = (int)mem_pointer / MC_CLICK;
	/* where in the click are we starting at */
	i = ((int)p % MC_CLICK) / UNIT_SZ;

	/* kludge to take into account the fact that mem_left is later
	 * subtracted by a whole click size, even if dump started in the
	 * middle of the click.
	 */
	mem_left += i * UNIT_SZ;

	/*
	 * Loop through memory (as indicated in the memory map
	 * in bitmap), writing what's there and skipping the holes.
	 */
	for (; count > 0; m++) {
		if (MC_MMAP(m, CD_LOC)) {
			for (; count > 0 && i < MC_CLICK/UNIT_SZ; i++) {
				/*
				 * Compress UNIT_SZ of memory at a time
				 */
				if (compress(fd,p) == -1) {
					printf("write error at 0x%x\n",p);
					return(FALSE);
				}
				p += UNIT_SZ;

				/* Calculate space remaining in device */
				count -= num_writes * K32;
				num_writes = 0;
			}
			i = 0;
			putchar('.');
		} else {
			p += MC_CLICK;
			update_map();
			putchar('x');
		}
		/*
		 * track how close we are to having looped through c_maxmem
		 * bytes.  This gets decremented regardless of holes, since
		 * c_maxmem is real mem + holes.
		 */
		mem_left -= MC_CLICK;
		if (mem_left == 0)
			break;
	}
	/* Done dumping on this device. Since cdump does not support dumps
	 * to multiple tape devices, if this device is a tape we're done.
	 */
	if (istape) {
		/* Flush data from the 32 Kb  outbuf */
		if (flush_buf(fd) == -1){
			printf("buffer flush failed\n");
			return(FALSE);
		}

		/* Write the dmesgs at the end of the dump */
		if (write_dmesg(fd) == -1){
			printf("write_dmesg failed\n");
			return(FALSE);
		}
	}
	else {
		/* Disk Device. If count is non-zero it means we are done
		 * dumping all memory and there is still free space on the
		 * device */
		if (count){
		 	if ((amount = flush_buf(fd)) == -1){
				printf("buffer flush failed\n");
				return(FALSE);
			}
		}
		/* if space still avlbl. after flushing buffer, write
		 * out the dmesgs.
		 */
		if ((count-amount) > 0) {
			write_dmesg(fd);
		}
	}
	mem_pointer = p;
	return (TRUE);
}

abort( abort_message )
	char *abort_message;
{
	printf("%s.  No Dump.\n", abort_message);
	return_to_firmware();
}

/*
 * return to firmware.  fiddles with the reboot information to allow
 * booting a kernel next. If booted with RB_DUMP set, the re_cfg_addr[1]
 * is set to zero (means put cfg structure of next program at high mem)
 * and the RB_DUMP flag is cleared (so we don't reboot the dumper).
 * If this program was not invoked with RB_DUMP set, the RB_HALT flag
 * is set and the re_cfg_addr[0] is cleared.
 */
return_to_firmware()
{
	struct sec_gmode modes;
	struct reboot rebootdata;
	struct sec_powerup *in;
	struct config_desc *cd = CD_LOC;

	/*
	 * Use normal exit for return to SSM
	 * (don't need to fiddle with flags because they
	 * will already be set correctly according to how
	 * dump was invoked)
	 */
	if (cd->c_cons->cd_type != SLB_SCSIBOARD) {
		exit(0);
		/*NOTREACHED*/
	}

	/*
	 * Get reboot structure.
	 */
	in = (struct sec_powerup *) cd->c_cons->cd_sc_init_queue;
	modes.gm_status = 0;
	rebootdata.re_powerup = 0;	/* 0 booted data, 1 powerup values */
	modes.gm_un.gm_board.sec_reboot = &rebootdata;
	in->pu_cib.cib_inst = SINST_GETMODE;
	in->pu_cib.cib_status = (int *)&modes;

	mIntr(cd->c_cons->cd_slic, 5, SDEV_SCSIBOARD);
	while((*in->pu_cib.cib_status & SINST_INSDONE) == 0)
		continue;

	if(*in->pu_cib.cib_status != SINST_INSDONE)
		printf("Cannot get Console Board modes\n");

	/*
	 * Now tell FW how to reboot
	 */
	rebootdata.re_boot_flag &= ~RB_DUMP;		/* turn off */
	if ((cd->c_boot_flag & RB_DUMP) == 0) {
		rebootdata.re_boot_flag |= RB_HALT;
		rebootdata.re_cfg_addr[0] = 0;
	} else
		rebootdata.re_cfg_addr[1] = 0;

	modes.gm_status = 0;
	in->pu_cib.cib_inst = SINST_SETMODE;
	in->pu_cib.cib_status = (int *)&modes;

	mIntr(cd->c_cons->cd_slic, 5, SDEV_SCSIBOARD);
	while((*in->pu_cib.cib_status & SINST_INSDONE) == 0)
		continue;

	if(*in->pu_cib.cib_status != SINST_INSDONE)
		printf("Cannot get Console Board modes\n");

	/*
	 * return to Firmware...
	 */
	in->pu_cib.cib_inst = SINST_RETTODIAG;
	in->pu_cib.cib_status = SRD_REBOOT;
	mIntr(cd->c_cons->cd_slic, 5, SDEV_SCSIBOARD);

	/*
	 * SCED will take control.
	 */
	for(;;)
		continue;
	/*NOTREACHED*/
}
