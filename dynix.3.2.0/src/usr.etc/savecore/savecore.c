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

#ifndef lint
static	char rcsid[] = "$Header: savecore.c 2.13 1991/09/10 22:07:04 $";
#endif

/*
 * savecore -- save system crash core dump
 *
 * based on:
 *	"@(#)savecore.c	4.13 (Berkeley) 83/07/02";
 * 
 * $Log: savecore.c,v $
 *
 *
 *
 *
 *
 *
 *
 *
 */

#include <stdio.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <machine/cfg.h>	/* for struct reboot and BNAMESIZE */
#include <machine/param.h>	/* for NBPG and CLSIZE => pagesize */
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include "sys/vtoc.h"
#include <stand/dump.h>
#include "savecore.h"

#define	DAY		(60L*60L*24L)
#define	LEEWAY		(3*DAY)
#define READ		0
#define WRITE		1
#define	TRUE		1
#define	FALSE		0
#define K1	(1024)
#define K32	(32 * K1)
#define K512	(512 * K1)
#define M1	(K1 * K1)
#define DUMP_DEV_SIZE	16 * M1		/* 16MB which is currently the   */
					/* smallest swap partition size. */
#define eq(a,b)		(strcmp(a,b) == 0)
#define dump_exists()	((di.dump_magic == (unsigned)DUMP_MAGIC || \
			di.dump_magic == (unsigned)NEW_MAGIC) && \
			di.dump_size > 0)
#define dump_compressed() (di.dump_magic == (unsigned)NEW_MAGIC)

#define ispart(c) (((c) >= 'a') && ((c) <= 'Z'))
#define valid_part(type) ((type) != V_NOPART && (type) != V_RESERVED)
#define SHUTDOWNLOG "/usr/adm/shutdownlog"

struct nlist nl[] = {
#define X_TIME		0
	{ "_time" },
#define X_VERSION	1
	{ "_version" },
#define X_PANICSTR	2
	{ "_panicstr" },
	{ "" },
};

char	cfg_copy[8192];
char	list_buf[8192];
struct	config_desc *cfg_ptr;
int	list_size;
int	fflag = FALSE;
int	dflag = 0, cflag = 0, qflag = 0,parallel = 0;
char	dflagarg[8192];
int	is_compressed = FALSE;
char	*cpend;
int	dump_size;
int	compressed_size;
int	dmesg_start;
int	dmesg_end;
char 	do_dmesg = TRUE;
short 	num_devs;
int 	seg_size = UNIT_SZ;
struct dumplist	flist;
unsigned int num_disksegs;
char 	*dfilename;
short 	num_compressors;


int	map_size;
int	chunk_size;
struct	dump_info di;			/* dump magic and size */
int	first_chunk_size;		/* size of first dump partition */
struct ioctl_reboot rb;			/* reboot structure from sec */
char	*systm = "/dynix";
char	*dmesgcmd = "/etc/dmesg > ";	/* First part of dmesg command */
int	badsys = FALSE;			/* True if system and dump kernel */
					/* don't match => disable further */
					/* nlist stuff and copying of kernel */
char	*dirname;			/* directory to save dumps in */
char	*pathname;			/* pathname of file save dumps in */
char	*ddname;			/* name of dump device */
char	*xdname;			/* other dump devices */
int	tape = FALSE;			/* true if dump resides on a tape */
time_t	dumptime;			/* time the dump was taken */
int	dumplo;				/* where dump starts on dump device */
int	xdumplo;			/* other dump offsets */
time_t	now;				/* current date */
char	*path();
char	*malloc(), *valloc();
char	*ctime();
#define MSTRLEN		80
char	vers[MSTRLEN+1];
char	core_vers[MSTRLEN+1];
unsigned vers_a;			/* addr of version in core */
int	vers_left = MSTRLEN;		/* # of char left to copy */
char	panic_mesg[MSTRLEN+1];
unsigned panicstr;
int	panic_left = MSTRLEN;		/* # of char left to copy */
char	*buf;
int	char_count;
char	*next_word();
off_t	lseek();
char	*console_sec();
static char curopt, *opts = "d:P:cCs:q";
extern char *optarg;
extern int optind, opterr;

main(argc, argv)
	char **argv;
	int argc;
{

	buf = valloc(K32);
	if (buf == (char *)NULL) {
		fprintf(stderr, "savecore: cannot valloc I/O buffer");
		exit(1);
	}

	time(&now);
	while (EOF != (curopt = getopt(argc, argv, opts))) {
		switch (curopt) {
		case 'c':
			cflag++;
			break;
		case 'P':
			num_compressors = atoi(optarg);
			break;
		case 'C':
			parallel++;
			break;
		case 's':
			seg_size = atoi(optarg);
			seg_size *= 1024;
			break;
		case 'q':
			qflag++;
			break;
		case 'd':
			strcpy(dflagarg, optarg);
			dflag++;
		}
	}
	if (cflag && qflag) {
		fprintf(stderr, "savecore: only one of \"-c\" or \"-q\".\n");
		exit(1);
	}
	if (cflag) {		/* zap dump */
		printf("savecore: Clearing dump.\n");
		read_sec();
		clear_dump();
		exit(0);
	} else if (qflag) {	/* check for dump */
		read_sec();
		if (check_for_dump()) {
			printf("Dump exists.\n");
			exit(0);
		} else {
			printf("No Dump.\n");
			exit(1);
		}
	}
	if (optind < argc) {
		dirname = argv[optind++];
	} else {
		fprintf(stderr, "usage: savecore [ -d dumpstring ] [ -C ] [ -P processes ] [ -s seg_size ] dirname [ system ]\n");
		fprintf(stderr, "       savecore [-d dumpstring ] -c\n");
		fprintf(stderr, "       savecore [-d dumpstring ] -q\n");
		exit(1);
	}
	if (optind < argc) {
		systm = argv[optind];
	}

	if (nlist(systm, nl) == -1)
		badsys = TRUE;
	vers_a = nl[X_VERSION].n_value;

	if (access(dirname, 2) < 0) {
		fprintf(stderr, "savecore: ");
		perror(dirname);
		exit(1);
	}
	read_sec();
	if (parallel)
		read_dlist();
	if (save_core())
		clear_dump();
	return 0;
}

clear_dump()
{
	register int dumpfd;

	dumpfd = open(ddname, 2);
	if (dumpfd < 0) {
		perror("savecore: Can't open dump device for read/write");
		exit(1);
	}
	if (!tape) {
		is_tape(dumpfd);
		skip_to_dump(dumpfd, READ, dumplo);
		if (read(dumpfd, buf, DEV_BSIZE) != DEV_BSIZE) {
			perror("savecore: Error reading dump");
			exit(1);
		}
		di = *((struct dump_info *) buf);
		if (dump_exists()) {
			skip_to_dump(dumpfd, WRITE, dumplo);
			((struct dump_info *)buf)->dump_magic = 0;
			if (write(dumpfd, buf, DEV_BSIZE) != DEV_BSIZE) {
				perror("savecore: Clear of dump failed");
				exit(1);
			}
		}
		close(dumpfd);
	}
}
/*
 * Are we talking to a tape?
 */
is_tape(fd)
int fd;
{
	char status[128];

	if (ioctl(fd, MTIOCGET, status) != -1)
		tape = TRUE;
}

/*
 * If we got an alternate dumpstring of the form "offset device" or
 * "-f dumplist" then we need to use this one instead.  After a couple
 * of tricks we parse just like read_sec().
 */
read_dflag()
{
	char *ds;			/* dump string */
	struct stat s;
	register char *cp;
	int fd, i;
	int psize;
	
	ds = cp = dflagarg;
	/*
	 * put nulls between all args of dumpstring so we can parse just
	 * like read_sec() does.
	 */
	for (; *cp != '\0'; cp++) {
		if (*cp == ' ' || *cp == '\t') {
			*cp = '\0';
		}
	}
	cp = ds;
	char_count = 0;

	if (eq(cp, "-f")) {
		cp = next_word(cp);
		dfilename = cp;
		if ( (fd = open(cp, 0)) < 0
		  || (list_size = read(fd, list_buf, sizeof(list_buf))) < 0
		  || close(fd) < 0
		   )
			nonsense(ds, "Error reading unix dump list");
		fflag = TRUE;
		cp = list_buf;
		for (cpend = cp; cpend < &list_buf[list_size]; cpend++)
			if (*cpend == '\n')
				break;
		if (cpend >= &list_buf[list_size])
			nonsense(ds, "Missing newline in unix dump list");
		*cpend = '\0';
		if (!standalone_name(cp))
			nonsense(list_buf, "Name of device to dump on is bad");
		cp = next_word(cp);
		if (cp == NULL || !decimal_integer(cp))
			nonsense(list_buf, "Offset isn't a decimal integer");
		dumplo = atoi(cp) * 512;
		cp = next_word(cp);
		if (cp == NULL)
			nonsense(list_buf, "Unix dump file doesn't exist");
		ddname = (char *)malloc(strlen(cp) + 1);
		strcpy(ddname, cp);
		for (i=0; ddname[i] != '\0'; i++) {
			if (ddname[i] == ' ' || ddname[i] == '\t') {
				ddname[i] = '\0';
				break;
			}
		}
		if (stat(ddname, &s) < 0)
			nonsense(list_buf, "Unix dump file doesn't exist");
		if ((psize = getpartsize(ddname,s)) == -1) {
			fprintf(stderr, 
			"savecore: Error: %s is an Invalid partition\n",ddname);
			exit(1);
		}
		cp = next_word(cp);
		if (cp != NULL || decimal_integer(cp))
			chunk_size = MIN(atoi(cp) * DEV_BSIZE, psize - dumplo);
		else
			chunk_size = psize - dumplo;
                if (chunk_size < 0)
                        chunk_size = 0;
                chunk_size &= ~(K32 - 1);
		return;
	} else {

		/*
		 * check and read dumplo
		 */

		if (!decimal_integer(cp))
			nonsense(ds, "Dump offset isn't a decimal integer");
		dumplo = atoi(cp) * 512;

		/*
		 * check and read unix dump name.
		 */

		cp = next_word(cp);
		if (stat(cp, &s) < 0)
			nonsense(ds, "Unix dump file doesn't exist");

		ddname = (char *)malloc(strlen(cp)+1);
		strcpy(ddname,cp);
	}
}

/*
 * determine dumplo, and ddname (where a dump
 * ought to be).  these can be gleaned out of the 
 * reboot structure of the scsi/ether controller.
 */
read_sec()
{
	int sec;			/* descriptor for /dev/smem */
	char *ds;			/* dump string */
	struct stat s;
	register char *cp;
	int fd, i;
	int psize, partno;
	int vfd;

	/*
	 * If we were given alternate dumpstring info, parse it instead
	 * of using the reboot info.
	 */
	if (dflag) {
		read_dflag();
		return;
	}

	/*
	 * get the half of the reboot structure for reboot name.
	 */

	rb.re_powerup = 0;			/* temp copy */
	sec = open(console_sec(), 0, 0);
	if (sec < 0) {
		perror("savecore: Can't open sced memory device");
		exit(1);
	}
	ioctl(sec, SMIOGETREBOOT1, &rb);
	close(sec);

	/*
	 * sanity check standalone dump program name
	 */

	ds = cp = rb.re_boot_name;
	char_count = 0;
	if (!standalone_name(cp)) 
		nonsense(ds, "Standalone dump program name is bad");

	/*
	 * sanity check where to dump name
	 */

	cp = next_word(cp);
	if (eq(cp, "-f")) {
		cp = next_word(cp);
		if (!standalone_name(cp))
			nonsense(ds, "Name of dump file on is bad");
		cp = next_word(cp);
		dfilename = cp;
		if ( (fd = open(cp, 0)) < 0
		  || (list_size = read(fd, list_buf, sizeof(list_buf))) < 0
		  || close(fd) < 0
		   )
			nonsense(ds, "Error reading unix dump list");
		fflag = TRUE;
		cp = list_buf;
		for (cpend = cp; cpend < &list_buf[list_size]; cpend++)
			if (*cpend == '\n')
				break;
		if (cpend >= &list_buf[list_size])
			nonsense(ds, "Missing newline in unix dump list");
		*cpend = '\0';
		if (!standalone_name(cp))
			nonsense(list_buf, "Name of device to dump on is bad");
		cp = next_word(cp);
		if (cp == NULL || !decimal_integer(cp))
			nonsense(list_buf, "Offset isn't a decimal integer");
		dumplo = atoi(cp) * 512;
		cp = next_word(cp);
		if (cp == NULL)
			nonsense(list_buf, "Unix dump file doesn't exist");
		ddname = (char *)malloc(strlen(cp) + 1);
		strcpy(ddname, cp);
		for (i=0; ddname[i] != '\0'; i++) {
			if (ddname[i] == ' ' || ddname[i] == '\t') {
				ddname[i] = '\0';
				break;
			}
		}
		if (stat(ddname, &s) < 0)
			nonsense(list_buf, "Unix dump file doesn't exist");

                /* read the VTOC from the dump partition. If no VTOC available
                 * or the chosen partition is invalid, skip this chunk.
                 */
		if ((psize = getpartsize(ddname,s)) == -1) {
			fprintf(stderr, 
			"savecore: Error: %s is an Invalid partition\n",ddname);
			exit(1);
		}
		cp = next_word(cp);
		if (cp != NULL || decimal_integer(cp))
			chunk_size = MIN(atoi(cp) * DEV_BSIZE, psize - dumplo);
		else
			chunk_size = psize - dumplo;
                if (chunk_size < 0)
                        chunk_size = 0;
                chunk_size &= ~(K32 - 1);
		return;
	}

	if (!standalone_name(cp))
		nonsense(ds, "Name of device to dump on is bad");

	/*
	 * check and read dumplo
	 */

	cp = next_word(cp);
	if (!decimal_integer(cp))
		nonsense(ds, "Dump offset isn't a decimal integer");
	dumplo = atoi(cp) * 512;

	/*
	 * check and read unix dump name.
	 */

	cp = next_word(cp);
	if (stat(cp, &s) < 0)
		nonsense(ds, "Unix dump file doesn't exist");

	ddname = (char *)malloc(strlen(cp)+1);
	strcpy(ddname,cp);
}

nonsense(d, s)
	register char *d;
	char *s;
{
	register int i;

	fprintf(stderr, "savecore: Warning: dump string `");
	if (d != NULL)
	for (i=0; i<BNAMESIZE; i++) {
		if (*d == '\0')
			putc(' ', stderr);
		else
			putc(*d, stderr);
		d++;
	}
	fprintf(stderr, "' does not make sense.\n");
	fprintf(stderr, "         (%s)\n", s);
	exit(1);
}

char *
next_word(cp)
	register char *cp;
{
	if (fflag == TRUE) {
		if ( cp == NULL )
			return ( NULL );
		while (*cp != ' ' && *cp != '\t') {
			if (++cp >= cpend)
				return ( NULL );
		}
		while (*cp == ' ' || *cp == '\t') {
			if (++cp >= cpend)
				return ( NULL );
		}
		return ( cp );
	}

	while (*cp != '\0') {				/* skip letters */
		if (char_count == BNAMESIZE-1)
			return(cp);
		cp++;
		char_count++;
	}
	while (*cp == '\0') {				/* skip nulls */
		if (char_count == BNAMESIZE-1)
			return(cp);
		cp++;
		char_count++;
	}
	return(cp);
}

/*
 * returns TRUE if the string pointed to by cp is a legal standalone name.
 */
standalone_name(cp)
	register char *cp;
{
	register int i;
	register int is_number;

	/*
	 * xx
	 */

	for (i=0; i<2; i++) {
		if (*cp < 'a' || *cp > 'z')
			return(FALSE);
		cp++;
	}

	/*
	 * xx(
	 */

	if (*cp++ != '(')
		return(FALSE);

	/*
	 * xx(nnn
	 */

	is_number = FALSE;
	while(*cp >= '0' && *cp <= '9') {
		cp++;
		is_number = TRUE;
	}
	if (is_number == FALSE)
		return(FALSE);

	/*
	 * xx(nn,
	 */

	if (*cp++ != ',')
		return(FALSE);

	/*
	 * xx(nn,nn
	 */

	is_number = FALSE;
	while(*cp >= '0' && *cp <= '9') {
		cp++;
		is_number = TRUE;
	}
	if (is_number == FALSE)
		return(FALSE);

	/*
	 * xx(nn,nn)
	 */

	if (*cp != ')')
		return(FALSE);

	return(TRUE);
}

/*
 * returns TRUE if cp points to a string containing a decimal integer
 * and nothing else.
 */
decimal_integer(cp)
	register char *cp;
{
	register int is_number = FALSE;

	while(*cp >= '0' && *cp <= '9') {
		cp++;
		is_number = TRUE;
	}
	if (is_number == FALSE)
		return(FALSE);
	if (*cp != '\0' && *cp != ' ' && *cp != '\t')
		return(FALSE);
	return(TRUE);
}

print_crashtime()
{

	if (!badsys || nl[X_TIME].n_value != 0) {
		printf("savecore: System went down at %s", ctime(&dumptime));
		if (dumptime < now - LEEWAY || dumptime > now + LEEWAY) {
			printf("savecore: Dump time is unreasonable\n");
		}
	}
	if (panicstr)
		printf("savecore: after panic: %s\n", panic_mesg);
}

char *
path(file)
	char *file;
{
	register char *cp = (char *)malloc(strlen(file) + strlen(dirname) + 2);

	(void) strcpy(cp, dirname);
	(void) strcat(cp, "/");
	(void) strcat(cp, file);
	return (cp);
}

check_space()
{
	struct statfs fsb;
	int spacefree;

	if (statfs(dirname, &fsb) < 0) {
		perror(dirname);
		exit(1);
	}
	spacefree = fsb.f_bfree * fsb.f_bsize / 1024;
	if (read_number("minfree") > spacefree) {
		fprintf(stderr,
		   "savecore: Dump omitted, not enough space on device\n");
		return (0);
	}
	if (fsb.f_bavail < 0)
		fprintf(stderr,
		"savecore: Dump performed, but free space threshold crossed\n");
	return (1);
}

read_number(fn)
	char *fn;
{
	char lin[80];
	register FILE *fp;

	if ((fp = fopen(path(fn), "r")) == NULL)
		return (0);
	if (fgets(lin, 80, fp) == NULL) {
		fclose(fp);
		return (0);
	}
	fclose(fp);
	return (atoi(lin));
}


save_core()
{
	register int n;
	char fname[512];
	register int ifd, ofd, bounds;
	register FILE *fp;
	char *cp, *dmesgout;

	ifd = open(ddname, 0);
	if (ifd < 0) {
		fprintf(stderr, "savecore: Can't open %s for reading.\n",
			ddname);
		exit(1);
	}
	/*
	 * Are we talking to a tape?
	 */
	is_tape(ifd);
	/*
	 * Get to beginning of dump
	 */
	skip_to_dump(ifd, READ, dumplo);
	if (read(ifd, buf, K32) != K32) {
		perror("savecore: Error reading dump");
		exit(1);
	}
	di = *((struct dump_info *) buf);
	if (!dump_exists())
		return(FALSE);
	if (!check_space())
		return(FALSE);
	if (dump_compressed())
		is_compressed = TRUE;
	if (!is_compressed){
		bcopy(&buf[(int)CD_LOC], cfg_copy, sizeof(cfg_copy));
		cfg_ptr = (struct config_desc *)cfg_copy;
		cfg_ptr->c_mmap = (u_long *)((u_long)cfg_ptr->c_mmap - (u_long)CD_LOC + (u_long)cfg_copy);
	}

	dump_size = di.dump_size;
	if (is_compressed) {
		compressed_size = di.compressed_size;
		dmesg_start = roundup(di.data_end,512);
		dmesg_end = di.dmesg_end;
		map_size = di.comp_size;
	}
#ifdef DEBUG
	printf("comp size = %d\n",compressed_size);
	printf("dmesg_start = %d\n",dmesg_start);
	printf("dmesg_end = %d\n",dmesg_end);
#endif DEBUG

	if (fflag){
		if (is_compressed)
			chunk_size = MIN(chunk_size, compressed_size);
		else
			chunk_size = MIN(chunk_size, dump_size);
	}

	first_chunk_size = chunk_size;
	/*
	 * Print where the dump came from
	 */
	if (dflag) {
		where_from(dflagarg);
	} else {
		where_from(rb.re_boot_name);
	}

	bounds = read_number("bounds");
	sprintf(fname, "vmcore.%d", bounds);
	if (parallel || is_compressed)
		strcat(fname, ".ZZ");
	pathname = path(fname);
	ofd = open(pathname,O_RDWR|O_CREAT);
	if (ofd < 0) {
		perror("savecore: Can't create dump file");
		exit(1);
	}
	fchmod(ofd,0640);

	if (is_compressed && !tape) { 
		/*print memory size --> compressed dump size*/
		printf("savecore: Saving core image (%d.%d MB --> %d.%d MB) in %s\n",
			dump_size / M1, (dump_size % M1) * 10 / M1,
			dmesg_end / M1, (dmesg_end % M1) * 10 / M1,
			fname);
	}
	else { /*tape device  or uncompressed dump*/
		printf("savecore: Saving core image (%d.%d MB) in %s\n",
			dump_size / M1, (dump_size % M1) * 10 / M1,
			fname);
	}	

	if (is_compressed)
		copy_compressed(ifd, ofd);
	else {
		if (parallel)
			pcopy(ddname,ofd,dump_size);
		else
			copydump(ifd, ofd);
	}
	/*
	 * Save dmesg output with crash dump.
	 */
		
	sprintf(fname, "dmesg.%d", bounds);
	dmesgout = path(fname);
	ofd = creat(dmesgout, 0644);
	if (ofd < 0) {
		perror("savecore: Can't create dmesg output file");
		exit(1);
	}
	if (dmesg_end - dmesg_start) {
		if (copy_dmesg(pathname,ofd) != -1)
			do_dmesg = FALSE;
	}
	if (do_dmesg) {
		close(ofd);
		cp = (char *)malloc(strlen(dmesgcmd) + strlen(dmesgout) + 1);
		(void) strcpy(cp, dmesgcmd);
		(void) strcat(cp, dmesgout);
		n = system(cp);
		if(n != 0) {
			fprintf(stderr,
				"savecore: Error saving dmesg.%d - error code %d.\n",
				bounds, n);
		}
	}
	if (!(is_compressed || parallel)){
	/*
	 * panic_left != 0 => haven't read panic msg
	 */
		if (panic_left && panicstr) {
			if ((ifd = open(ddname, 0)) < 0) {
			fprintf(stderr, "savecore: Can't open %s for reading.\n",
				ddname);
			exit(1);
			}
			is_tape(ifd);
			skip_to_dump(ifd, READ, dumplo);
			reread_forpanic(ifd);
			close(ifd);
		}

		check_dump();
	}
	if (!badsys) {
		ifd = open(systm, 0);
		sprintf(fname, "dynix.%d", bounds);
		ofd = creat(path(fname), 0644);
		if (ofd < 0) {
			perror("savecore: Can't create dynix image file");
			exit(1);
		}
		while((n = read(ifd, buf, BUFSIZ)) > 0)
			write(ofd, buf, n);
		close(ifd);
		close(ofd);
	} else
			printf("savecore: Warning: not saving dynix image\n");

		fp = fopen(path("bounds"), "w");
		fprintf(fp, "%d\n", bounds+1);
		fclose(fp);
		return(TRUE);
	}

/*
 * skip to start of the dump. (can't lseek on tapes)
 */
skip_to_dump(fd, mode, offset)
	int fd;
	int mode;
	int offset;
{
	int sofar;
	extern int read(), write();
	int (*f)();

	if (!tape) {
		if (lseek(fd, offset, L_SET) == -1) {
			perror("savecore: Error on lseek to start of dump");
			exit(1);
		}
	}
}

/*
 * copy the dump from ifd to ofd
 *
 * Since either input and/or output devices may be a tape, savecore now
 * looks for the dumptime, version, etc while copying the dump and will
 * rescan the dump for the panic message if necessary.
 */

copydump(ifd, ofd)
	int ifd;
	int ofd;
{
	register int i;
	register unsigned mem, copied;
	int m;
	if ( fflag == TRUE ) {
		printf(" %d.%d MB from %s",
			chunk_size / M1, (chunk_size % M1) * 10 / M1, ddname);
		fflush(stdout);
	}

	/*
	 * loop through all of the dump
	 * First 32 KB are already in buf.
	 */

	mem = dump_size;	/* 32K granularity */
	copied = 0;
	m = 0;
	for (; copied < mem; m++) {
		if (MC_MMAP(m,cfg_ptr)) {
			for (i = 0; copied < mem && i < MC_CLICK/K32; i++) {
				if (copied != 0 && read(ifd, buf, K32) != K32) {
					perror("savecore: read error during dump copy");
					exit(1);
				}
				/* 
				 * See if dump_time, version, or panicstr/panicmsg
				 * need to be extracted from this buffer
				 */
				check_for_kvars(copied);
				if (write(ofd, buf, K32) != K32) {
					perror(
				"savecore: read error during dump copy");
					exit(1);
				}
				copied += K32;
				chunk_size -= K32;
				if (copied >= mem)
					break;
				/*
				 * Handle multiple dump partitions
				 */
				if (fflag)
				while (chunk_size <= 0) {
					(void) close(ifd);
					if (!next_chunk(copied)) {
						printf("\n");
						close(ifd);
						fsync(ofd);
						fsync(ofd);
						if (copied < mem)
							fprintf(stderr,
					"savecore: Warning: Partial dump taken. \n");
						return;
					}
						
							
					ifd = open(xdname, 0);
					if (ifd < 0) {
						fprintf(stderr, 
					"savecore: Can't open %s for reading.\n",
							xdname);
						exit(1);
					}
					/*
					 * Are we talking to a tape?
					 */
					is_tape(ifd);
					/*
					 * Get to beginning of dump
					 */
					skip_to_dump(ifd, READ, xdumplo);
					printf("\n %d.%d MB from %s",
						chunk_size / M1, 
						(chunk_size%M1)*10/M1, 
						xdname);
					fflush(stdout);
				}
			}
			printf(".");
			fflush(stdout);
		} else {
			(void) lseek(ofd, (off_t)MC_CLICK, L_INCR);
			copied += MC_CLICK;
			printf("x");
			fflush(stdout);
		}
	}
	printf("\n");
	close(ifd);
	fsync(ofd);	/* force dumped blocks to disk */
	close(ofd);
}

copy_compressed(ifd, ofd)
	int ifd;
	int ofd;
{
	register int sz,amount;
	register unsigned mem, copied;
	unsigned header_size;
	char *buffer;
	if ( fflag == TRUE ) {
		printf(" %d.%d MB from %s",
			chunk_size / M1, (chunk_size % M1) * 10 / M1, ddname);
		fflush(stdout);
	}

	/*
	 * loop through all of the dump
	 * First 32 KB are already in buf.
	 */
	mem = compressed_size;	/* 32K granularity */
	copied = 0;

	if (tape) { /* dump device is a tape; "-f" not supported
		     compression header located at the end of the vmcore file*/

		/*write out the 32k already read*/
		if (write(ofd, buf, K32) != K32) {
			perror("savecore: write error during dump copy");
			exit(1);
		}

		/*transfer rest of the file from the tape*/
		while((sz = read(ifd, buf, K32)) != -1){
			if (write(ofd, buf, sz) != sz) {
				perror("savecore: write error during dump copy");
				exit(1);
			}
		}
		header_size = roundup((sizeof(struct dump_info)+map_size),DEV_BSIZE);
		if ((buffer = malloc(header_size)) == 0){
			printf("savecore:buffer malloc failed");
			exit(1);
		}

		/*seek back to start of header*/
		if (lseek(ofd, -1*header_size,L_INCR) == -1){
			perror("savecore:error seeking during dump copy");
			exit(1);
		}

		/*read the header*/
		if (read(ofd,buffer,header_size) == -1){
			perror("savecore: read error during dump copy");
			exit(1);
		}

		/*read the location of dmesg*/
		dmesg_start = roundup(((struct dump_info *)buffer)->data_end,
						512);
		dmesg_end = ((struct dump_info *)buffer)->dmesg_end;

		/*write the header at the start of the vmcore file*/
		lseek(ofd,0,0);
		if (write(ofd,buffer,header_size) == -1) {
			perror("savecore: write error during dump copy");
			exit(1);
		}

		/*cut out the header from the vmcore file*/
		if (ftruncate(ofd,dmesg_end) == -1) 
			perror("savecore: write error during dump copy");
	}
	else { /*not tape; "-f supported
		header at the beginning of vmcore file*/

	 	while(TRUE) {
			/*read skipped on first pass*/
			if (copied != 0)  
				if (read(ifd, buf, K32) != K32) {
					perror("read error during dump copy");	
					exit(1);
					}
			if (write(ofd, buf,K32) != K32) {
				perror("savecore: write error during dump copy");
				exit(1);
			}
			copied += K32;
			chunk_size -= K32;
			if (copied >= mem)
				break;
			/*
		 	* Handle multiple dump partitions
		 	*/
			if (fflag)
				while (chunk_size <= 0) {
					(void) close(ifd);
					if (!next_chunk(copied)) {
						printf("\n");
						close(ifd);
						fsync(ofd);
						fsync(ofd);
						if (copied < mem)
							fprintf(stderr,
					"savecore: Warning: Partial dump taken. \n");
						return;
					}
					ifd = open(xdname, 0);
					if (ifd < 0) {
						fprintf(stderr, 
						"savecore: Can't open %s for reading.\n", xdname);
						exit(1);
					}
					/*
				 	* Are we talking to a tape?
				 	*/
					is_tape(ifd);
					/*
				 	* Get to beginning of dump
				 	*/
					skip_to_dump(ifd, READ, xdumplo);
					printf("\n %d.%d MB from %s",
						chunk_size / M1, 
						(chunk_size%M1)*10/M1, 
						xdname);
					fflush(stdout);
				}
			if (!((copied / K32) % 8))
				printf(".");
			fflush(stdout);
		} 
	}
	printf("\n");
	close(ifd);
	fsync(ofd);	/* force dumped blocks to disk */
	close(ofd);
}

copy_dmesg(pathname,ofd)
	char *pathname;
	int ofd;
{
	int ifd;
	int size = dmesg_end - dmesg_start;
	char *dmesg_buf;

#ifdef DEBUG
	printf("Copy_dmesg: dmesg_start = %d size = %d\n",dmesg_start,size);
#endif DEBUG

	if ((ifd = open(pathname,O_RDWR)) == -1){
		perror("copy_dmesg:open of core failed\n");
		return(-1);
	}
	/*seek to start of dmesg*/
	if (lseek(ifd,dmesg_start,L_SET) == -1) {
		perror("copy_dmesg:error seeking during dmesg copy");
		return(-1);
	}
	if ((dmesg_buf = malloc(size)) == 0) {
		perror("copy_dmesg:error reading during dmesg copy");
		return(-1);
	}
	if (read(ifd,dmesg_buf,size) == -1) {
		perror("copy_dmesg:error reading during dmesg copy");
		return(-1);
	}
	if (write(ofd,dmesg_buf,size) == -1) {
		perror("copy_dmesg:error writing during dmesg copy");
		return(-1);
	}
	/*Reduce the file to size -  dmesg_end*/
	/*Thus the core file will finally contain the memory dump
	 and the dmesg stuff*/

	if (ftruncate(ifd,dmesg_end) == -1) {
		perror("copy_dmesg:error truncating core file");
	}
	close(ifd);
	fsync(ofd);
	close(ofd);
}

/*
 * Get the next line from the dump file
 */
next_chunk(copied)
	int copied;
{
	register char *cp;
	register int i;
	struct stat s;
	int mem_left;
	int psize, partno;
	int fd;

	if (is_compressed)
		mem_left = compressed_size - copied;
	else
		mem_left = dump_size - copied;

	cp = cpend + 1;
	if (cp >= &list_buf[list_size])
		return(0);	
	for (cpend = cp; cpend < &list_buf[list_size]; cpend++)
		if (*cpend == '\n')
			break;
	if (cpend >= &list_buf[list_size])
		nonsense(cp, "Missing newline in unix dump list");
	*cpend = '\0';
	if (!standalone_name(cp))
		nonsense(cp, "Name of device to dump on is bad");
	cp = next_word(cp);
	if (cp == NULL || !decimal_integer(cp))
		nonsense(cp, "Offset isn't a decimal integer");
	xdumplo = atoi(cp) * 512;
	cp = next_word(cp);
	if (cp == NULL)
		nonsense(cp, "Unix dump file doesn't exist");
	xdname = (char *)malloc(strlen(cp) + 1);
	strcpy(xdname, cp);
	for (i=0; xdname[i] != '\0'; i++) {
		if (xdname[i] == ' ' || xdname[i] == '\t') {
			xdname[i] = '\0';
			break;
		}
	}
	if (stat(xdname, &s) < 0)
		nonsense(xdname, "Unix dump file doesn't exist");
        /*
         * Size of this chunk is the minima of size parameter,
         * the partition size(minus xdumplo), and the memory left to
         * save.
         */
	if ((psize = getpartsize(xdname,s)) == -1) {
		fprintf(stderr,
		"\nsavecore: Error: %s is an Invalid partition\n",xdname);
		exit(1);
	}
        cp = next_word(cp);
        if (cp != NULL && decimal_integer(cp))
                chunk_size = MIN(atoi(cp) * DEV_BSIZE, mem_left);
        else
                chunk_size = mem_left;
        chunk_size = MIN(chunk_size, psize - xdumplo);
        if (chunk_size < 0)
                chunk_size = 0;

	chunk_size &= ~(K32 - 1);
	return(1);
}

/*
 * reread the dump looking for panic_mesg
 */
reread_forpanic(ifd)
	int ifd;
{
	register unsigned mem, scanned;

	mem = first_chunk_size;		/* 32K granularity */
	scanned = 0;
	while (scanned < mem) {
		/* 
		 * See if dump_time, version, or panicstr/panicmsg
		 * need to be extracted from this buffer
		 */
		if (read(ifd, buf, K32) != K32) {
			perror("savecore: read error during dump copy");
			exit(1);
		}
		if (panicstr != 0 && panicstr >= scanned && panicstr < scanned+K32) {
			copystr(&panicstr, scanned,
				panic_mesg + MSTRLEN - panic_left, &panic_left);
			/* if we've got it, don't waste time reading more */
			if (panic_left == 0){
				return;
}
		}
		scanned += K32;
	}
}


/*
 * While the dump is being copied, this routine attempts to extract
 * the few values from the dump that are needed.  The old version used
 * to seek around on the copied dump to get them, but this didn't work
 * if the dump was copied to a character device (e.g., raw tape).
 * (The above is accomplished by making vmcore.N a symbol link to a raw
 *  tape device.)
 * For the version and panic message, there is a copy of the address
 * of that item that is bumped as characters are copied.  This is maintained
 * to facilitate extracting strings that cross 32K boundaries.
 */
check_for_kvars(addr)
	register unsigned addr;
{
	register unsigned kva;

	kva = nl[X_TIME].n_value;
	if (kva != 0 && kva >= addr && kva < addr+K32) {
		dumptime = *((time_t *) &buf[kva-addr]);
	}
	kva = nl[X_PANICSTR].n_value;
	if (kva != 0 && kva >= addr && kva < addr+K32) {
		panicstr = *((int *) &buf[kva-addr]);
	}
	/*
	 * The address for the core_vers has been set previously
	 */
	if (vers_a != 0 && vers_a >= addr && vers_a < addr+K32) {
		copystr(&vers_a, addr, core_vers + MSTRLEN - vers_left,
			 &vers_left);
	}
	if (panicstr != 0 && panicstr >= addr && panicstr < addr+K32) {
		copystr(&panicstr, addr, panic_mesg + MSTRLEN - panic_left,
			 &panic_left);
	}
}

copystr(core_addr, buf_addr, s, limit)
	register unsigned *core_addr;
	register unsigned buf_addr;
	register char *s;
	register int *limit;
{
	while (*limit) {
		if ((*s = buf[*core_addr - buf_addr]) == '\0') {
			*limit = 0;
			return;
		}
		(*core_addr)++;
		(*limit)--;
		s++;
		/* reached 32 K boundary? */
		if (((*core_addr) & (K32-1)) == 0)
			return;
	}
}

check_dump()
{
	check_vers();
	log_entry();
	print_crashtime();
}

check_vers()
{
	FILE *fp;

	if (nl[X_VERSION].n_value != 0) {
		if ((fp = fopen(systm, "r")) == NULL) {
			fprintf(stderr, "savecore: Couldn't fopen %s\n",
				systm);
			badsys = TRUE;
			return;
		}
		/*
		 * This assumes that kernel is a stand-alone object (SMAGIC)!
		 */
		fseek(fp, (long)nl[X_VERSION].n_value, 0);
		fgets(vers, sizeof vers, fp);
		fclose(fp);

		if (!eq(vers, core_vers)) {
			fprintf(stderr,
			   "savecore: Warning: dynix version mismatch:\n\t%sand\n\t%s",
			   vers, core_vers);
			badsys = TRUE;
		}
	} else {
		badsys = TRUE;
		fprintf(stderr, "savecore: Warning: version not in %s\n",
			systm);
	}
}

/*
 * print dump came from
 */
where_from(p)
register char *p;
{
	int save_fflag = FALSE;

	printf("Dump from: ");
	char_count = 0;		/* for next_word() */
	if (fflag == TRUE) {
		save_fflag = TRUE;
		fflag = FALSE;
	}
	while (*p && (char_count < BNAMESIZE-1)) {
		printf("%s ", p);
		p = next_word(p);
	}
	printf("\n");
	if (save_fflag == TRUE)
		fflag = TRUE;
}


/*
 * return a pointer to the name of the scsi/ether the console is on.
 * /dev/smemco always maps to the console scsi/ether controller.
 */
char *
console_sec()
{
	return("/dev/smemco");
}

char *days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec"
};

log_entry()
{
	FILE *fp;
	struct tm *tm, *localtime();

	if (now == 0) 
		time(&now);
	tm = localtime(&now);
	fp = fopen("/usr/adm/shutdownlog", "a");
	if (fp == 0)
		return;
	fseek(fp, 0L, 2);
	fprintf(fp, "%02d:%02d  %s %s %2d, %4d.  Reboot", tm->tm_hour,
		tm->tm_min, days[tm->tm_wday], months[tm->tm_mon],
		tm->tm_mday, tm->tm_year + 1900);
	if (panicstr)
		fprintf(fp, " after panic: %s\n", panic_mesg);
	else
		putc('\n', fp);
	fclose(fp);
}

/*
 * Check for dump present (-q flag)
 * Some code is duplicated here (seemed clearer).
 */

check_for_dump()
{
	register int ifd;
	ifd = open(ddname, 0);
	if (ifd < 0) {
		fprintf(stderr, "savecore: Can't open %s for reading.\n",
			ddname);
		exit(1);
	}
	/*
	 * Are we talking to a tape?
	 */
	is_tape(ifd);
	/*
	 * Get to beginning of dump
	 */
	skip_to_dump(ifd, READ, dumplo);

	if (read(ifd, buf, K32) != K32) {
		perror("savecore: Error reading dump");
		exit(1);
	}
	close(ifd);
	di = *((struct dump_info *) buf);
	return (dump_exists());
}

/* parse /etc/DUMPLIST into a struct dumplist. This structure is used
 * by the compress processes for mapping the linear view into a multiple 
 * partition view 
 */

read_dlist()
{
	char *filename;
	int offset;
	int size;
	int i;
	int line;
	int num_segs;
	register char *cp;
	struct stat s;
	int fd,vfd;
	int psize, partno;

	if (fflag) {
		if ( (fd = open(dfilename, 0)) < 0
		  || (list_size = read(fd, list_buf, sizeof(list_buf))) < 0
		  || close(fd) < 0
		   )
			nonsense("", "Error reading unix dump list");
		fflag = TRUE;
		for (cp=list_buf; cp < &list_buf[list_size]; cp++){
			if (*cp == '\n') {
				num_devs++;
			}	
		}
		flist.ifname = (char **)(malloc(num_devs * sizeof(int)/sizeof(char)));
		flist.offset = (int *)(malloc(num_devs * sizeof(int)/sizeof(char)));
		flist.n_segs = (int *)(malloc(num_devs * sizeof(int)/sizeof(char)));
		cp = list_buf;
		line = 0;
		while (cp < &list_buf[list_size]) {
			for (cpend = cp; cpend < &list_buf[list_size]; cpend++)
				if (*cpend == '\n')
					break;
			if (cpend >= &list_buf[list_size])
				nonsense("", "Missing newline in unix dump list");
			*cpend = '\0';
			if (!standalone_name(cp))
				nonsense(list_buf, "Name of device to dump on is bad");
			cp = next_word(cp);
			if (cp == NULL || !decimal_integer(cp))
				nonsense(list_buf, "Offset isn't a decimal integer");
			offset = atoi(cp) * 512;
			flist.offset[line] = offset;
			cp = next_word(cp);
			if (cp == NULL)
				nonsense(list_buf, "Unix dump file doesn't exist");
			filename = (char *)malloc(strlen(cp) + 1);
		        strcpy(filename, cp);

			for (i=0; filename[i] != '\0'; i++) {
				if (filename[i] == ' ' || filename[i] == '\t') {
					filename[i] = '\0';
					break;
				}
			}
			if (stat(filename, &s) < 0)
				nonsense(list_buf, "Unix dump file doesn't exist");
			flist.ifname[line] = filename;
                	if ((psize = getpartsize(filename,s)) == -1){
				fprintf(stderr, 
			"savecore: Error: %s is an Invalid partition\n",filename);
				exit(1);
			}

			cp = next_word(cp);
			if (cp != NULL || decimal_integer(cp))
				size = MIN(atoi(cp) * 512, psize - offset);
        		else if ( offset < DUMP_DEV_SIZE )
                		size = psize - offset;
        		else
                		size = 0;
			if ( size < 0)
				size = 0;
        		size &= ~(K32 - 1);
cont:
			num_segs = size / seg_size;
			flist.n_segs[line] = num_segs;
			num_disksegs += num_segs;
			line++;
			cp = cpend + 1;

		}

	}
		
}

getpartsize(fname,s)
	char *fname;
	struct stat s;
{
	int vfd;
	char *p, *buf, c;
	int  len;
	int partno,psize;
	struct vtoc *vt;
	extern char *rindex();

	if ((buf = malloc(128)) == 0)
		return(-1);
	if ((p = rindex(fname, '/')) == 0)
		p = fname;
	else
		p++;

	if (*p == 'r')
		++p;
	(void) sprintf(buf, "/dev/r%s", p);
	len = strlen(buf);

	/* Trim partition from name */
		buf[len-1] = '\0';
	fname = buf;

	if ((vfd = open(fname, O_RDWR)) < 0) {
		fprintf(stderr, "savecore: Can't open %s for reading.\n",
				fname);
		exit(1);
	}
        if ((vt = (struct vtoc *)valloc(V_SIZE)) == NULL) {
		fprintf(stderr,"savecore: out of memory");
		exit(1);
	}

	if (ioctl(vfd, V_READ, vt) < 0) {

		/* unable to read vtoc */
		close(vfd);
		psize = DUMP_DEV_SIZE;
		return(psize);
		
	}
	if (vt->v_sanity == VTOC_SANE) {

		/* a valid vtoc found */
		close(vfd);
		partno = VPART(s.st_rdev);
		if (partno >= vt->v_nparts) {
			return(-1);
		}

		if (valid_part(vt->v_part[partno].p_type)) {
			psize = vt->v_part[partno].p_size * DEV_BSIZE;
		} else {
			return(-1);
		}
	} else {

		/* vtoc hosed */
		close(vfd);
		psize = DUMP_DEV_SIZE;
	}

	return(psize);
}
