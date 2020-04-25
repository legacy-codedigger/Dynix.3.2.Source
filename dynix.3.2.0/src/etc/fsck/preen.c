/*
 * $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ifndef STANDALONE
#ident "$Header: preen.c 1.2 90/02/20 $"

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <signal.h>
#include <fstab.h>

#define BASESZ	128
#define PATHSZ	MAXPATHLEN

/* physical disk info structure */
struct phys_info {
	struct phys_info *phys_next;	/* linked list */
	struct part_info *phys_part;	/* partition info (head) */
	struct part_info *phys_part_tl;	/* partition info tail pointer */
	struct part_info *phys_cur;	/* the partition being worked on */
	char	phys_name[BASESZ];	/* base name of drive */
	int	phys_pid;		/* PID of fsck checking this device */
};
struct phys_info *phys_head;		/* list head */
struct phys_info *phys_tail;		/* list tail */
struct part_info *bad_parts;		/* list of bad partitions */

/* partition info */
struct part_info {
	struct part_info *part_next;	/* linked list */
	struct part_info *part_bad;	/* bad linked list */
	char	part_name[PATHSZ];	/* device special path name for fsck */
};

extern int	errno;
extern char	*malloc();
extern void	catch();
extern void	voidquit();
extern int	returntosingle;
extern int	fork();
extern void	perror();
extern void	exit();
extern char	*blockcheck();

/*
 * preen_list
 *
 * Create a linked list of physical disks that each contain a list of
 * partitions for that disk.  Spawn a parallel process for each physical
 * device.  Avoid seeks by applying 'function' to each partition serially.
 */

static void
preen_list(function)
	int	(*function)();		/* function to call */
{
	register struct phys_info *phys;
	int	sumstatus = 0;
	int	pid;
	int	w_status;

	(void)signal(SIGINT, SIG_DFL);

	/*
	 * Start a child for each device.
	 */
	for (phys = phys_head; phys; phys = phys->phys_next) {
		next_part(phys, function);
	}

	/*
	 * Wait for children.  Collect status, and start a new child
	 * as long as there's work to do. Print a list of failing
	 * partitions.  Exit with the max status collected
	 * from all child exits.
	 */

	errno = 0;
	while ((pid = wait(&w_status)) > 0 || errno == EINTR) {
#ifdef	DEBUG
		printf("waitfor(pid=%d, w_status = 0x%x):\n", pid, w_status);
		(void)fflush(stdout);
#endif	/* DEBUG */
		if (errno == EINTR) {
			errno = 0;
			continue;
		}
		sumstatus |= (w_status >> 8);
		for (phys = phys_head; phys; phys = phys->phys_next) {
			if (pid == phys->phys_pid)
				break;
		}
		if (phys == NULL) {
			printf("Unknown pid %d\n", pid);
			continue;
		}
		if (w_status != 0) {
			phys->phys_cur->part_bad = bad_parts;
			bad_parts = phys->phys_cur;
		}
		next_part(phys, function);
	}

#ifdef DEBUG
	switch (sumstatus) {
	case 0:
		(void) printf("File systems OK\n");
		break;
	case 4:
		(void) printf("*** Reboot UNIX (no sync!)\n");
		break;
	case 8:
	default:
		(void) printf("*** Preen failed ... help!\n");
		break;
	case 12:
		(void) printf("*** Preen interrupted\n");
		break;
	}
	(void)fflush(stdout);
#endif /* DEBUG */
	if (sumstatus) {
		if (bad_parts == NULL) {
			printf("non-zero sumstatus with no bad parts %d\n",
				sumstatus);
			exit(8);
		}
		printf("THE FOLLOWING FILE SYSTEM(S) HAD UNEXPECTED PROBLEMS\n");
		while (bad_parts != NULL) {
			printf("%s\n", bad_parts->part_name);
			bad_parts = bad_parts->part_bad;
		}
	}
	if (returntosingle) {
		printf("A QUIT signal was received during preen\n");
		sumstatus |= 2;
	}
	exit(sumstatus);
}

next_part(phys, function)
	struct  phys_info *phys;
	int	(*function)();		/* function to call */
{

	/*
	 * If there's no more work to do, return.
	 */
	if (phys->phys_cur == phys->phys_part_tl) {
		phys->phys_pid = 0;
		return;
	}
		
	/*
	 * If this is the first call, choose the first
	 * partition off the list.  Otherswise, choose
	 * the next partition.
	 */
	if (phys->phys_cur == NULL)
		phys->phys_cur = phys->phys_part;
	else
		phys->phys_cur = phys->phys_cur->part_next;

	if ((phys->phys_pid = fork()) < 0) {
		perror("fork");
		exit(8);
	}
	if (phys->phys_pid != 0)
		return;
	/*
	 * call appropriate function and
	 * collect exit status...
	 */
#ifdef TESTING
	printf("next_part pid %d device %s\n", getpid(),
		phys->phys_cur->part_name);
	sleep((random() & 0xF) + 1);
	printf(" ...%d done\n", getpid());
	exit(0);
#else
	(void)signal(SIGQUIT, voidquit);
	(void)signal(SIGINT, catch);
	exit((*function)(phys->phys_cur->part_name));
#endif
	/* NOTREACHED */
}


/*
 * Convert a disk name (either raw or block) to the base name of the disk.
 * This is useful for finding the physical device associated with a
 * partition.
 *
 * Examples of conversion might be:
 *	/dev/rdsk/zd0s0	yields	zd0
 *	/dev/dsk/zd0s6	yields	zd0
 *	/dev/rzd0a	yields	zd0
 *	/dev/zd0g	yields	zd0
 */

static
char *
DiskBaseName(name)
	char	*name;
{
	register char	*b, *e;		/* begin and end of BaseName */
	static char	newnm[BASESZ];	/* don't trash 'name' */
	extern char *rindex();

	b = rindex(name, '/');
	b = b ? b + 1 : name;		/* if no '/', already basename */
	if (*b == 'r')
		b++;

	e = b;
	while (isalpha(*e))
		e++;
	while (isdigit(*e))
		e++;
	if (*e == '\0')
		return(b);
	(void)strncpy(newnm, b, e - b);
	newnm[e - b] = '\0';
	return(newnm);
}


/*
 * Add a disk to the linked list of physical drives.  If that drive
 * has already been added, return a pointer to it.
 * 'diskname' is the physical device.
 */

static
struct phys_info *
AddDisk(diskname)
	char	*diskname;
{
	register struct phys_info *phys;

	/* now search the existing list */
	for (phys = phys_head; phys; phys = phys->phys_next) {
		if (strcmp(phys->phys_name, diskname) == 0)
			return(phys);	/* found it */
	}

	/* make a new entry */
	phys = (struct phys_info *) malloc(sizeof(struct phys_info));
#ifdef DEBUG
	printf("AddDisk(): sizeof(phys_info) = %d\n", sizeof(struct phys_info));
	printf("AddDisk(): phys = 0x%x\n", phys);
	(void)fflush(stdout);
#endif /* DEBUG */
	if (phys == NULL) {
		perror("Out of memory (phys_info)");
		exit(8);
	}
	phys->phys_next = NULL;
	phys->phys_part = NULL;
	phys->phys_part_tl = NULL;
	phys->phys_cur = NULL;
	(void)strcpy(phys->phys_name, diskname);
	phys->phys_pid = 0;
	if (phys_tail) {
		phys_tail->phys_next = phys;
		phys_tail = phys;
	} else
		phys_head = phys_tail = phys;
	return(phys);
}


/*
 * Add a partition descripton to list associated with 'phys'.
 * 'specname' is the name of the device special file to be checked.
 */

static
AddSlice(phys, specname)
	register struct phys_info *phys;
	char	*specname;
{
	register struct part_info *part;

	part = (struct part_info *) malloc(sizeof(struct part_info));
	if (part == NULL) {
		perror("Out of memory (part_info)");
		exit(8);
	}
	part->part_next = NULL;
	(void)strcpy(part->part_name, specname);

	if (phys->phys_part_tl) {
		phys->phys_part_tl->part_next = part;
		phys->phys_part_tl = part;
	} else
		phys->phys_part = phys->phys_part_tl = part;
}

/*
 * Interface to the new preen algorithm
 */
newpreen()
{
	struct phys_info *phys;
	struct fstab *fsp;
	char *name;
	extern void checkfilesys();

#ifdef TESTING
	srandom(time((long *)0));
	setlinebuf(stdout);
#endif
	if (setfsent() == 0)
		errexit("Can't open checklist file: %s\n", FSTAB);
	while ((fsp = getfsent()) != 0) {
		if (strcmp(fsp->fs_type, FSTAB_RW) &&
		    strcmp(fsp->fs_type, FSTAB_RO) &&
		    strcmp(fsp->fs_type, FSTAB_RQ))
			continue;
		name = blockcheck(fsp->fs_spec);
		if (name == 0)
			continue;
		if (fsp->fs_passno == 1) {
#ifdef TESTING
			printf("Pass 1: check %s\n", name);
#else
			checkfilesys(name);
#endif
			continue;
		}
		phys = AddDisk(DiskBaseName(name));
		AddSlice(phys, name);
	}
	endfsent();
	preen_list(checkfilesys);
	/*NOTREACHED*/
}
#endif /* STANDALONE */
