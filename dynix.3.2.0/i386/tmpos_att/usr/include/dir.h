/* <dir.h> -- definitions for 4.2BSD-compatible directory access */

#define	DIRBLKSIZ	2600		/* size of directory block */
#define	MAXNAMLEN	255		/* maximum filename length */
	/* NOTE:  MAXNAMLEN must be one less than a multiple of 4 */

struct direct				/* data from readdir() */
	{
	long		d_ino;		/* inode number of entry */
	unsigned short	d_reclen;	/* length of this record */
	unsigned short	d_namlen;	/* length of string in d_name */
	char		d_name[MAXNAMLEN+1];	/* name of file */
	};

typedef struct
	{
	int	dd_fd;			/* file descriptor */
	int	dd_loc;			/* offset in block */
	int	dd_size;		/* amount of valid data */
	char	dd_buf[DIRBLKSIZ];	/* directory block */
	}	DIR;			/* stream data from opendir() */

extern DIR		*opendir();
extern struct direct	*readdir();
extern long		telldir();
extern void		seekdir();
extern void		closedir();

#define rewinddir( dirp )	seekdir( dirp, 0L )

#ifndef NULL
#define	NULL	0
#endif
