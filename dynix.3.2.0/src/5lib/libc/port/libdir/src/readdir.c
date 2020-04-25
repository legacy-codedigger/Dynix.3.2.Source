/*
	readdir -- C library extension routine for non-BSD UNIX

	last edit:	09-Jul-1983	D A Gwyn
*/

#include	<dir.h>
#include	<sys/types.h>

extern char	*strncpy();
extern int	read(), strlen();

#define NULL	0

#define DIRSIZ	256
struct olddir
	{
	ino_t	od_ino; 		/* inode */
	char	od_name[DIRSIZ];	/* filename */
	};

struct direct *
readdir( dirp )
	register DIR		*dirp;	/* stream from opendir() */
	{
	register struct olddir	*dp;	/* -> directory data */

	for ( ; ; )
		{
		if ( dirp->dd_loc >= dirp->dd_size )
			dirp->dd_loc = dirp->dd_size = 0;

		if ( dirp->dd_size == 0 	/* refill buffer */
		  && (dirp->dd_size = read( dirp->dd_fd, dirp->dd_buf, 
					    DIRBLKSIZ
					  )
		     ) <= 0
		   )
			return NULL;	/* error or EOF */

		dp = (struct olddir *)&dirp->dd_buf[dirp->dd_loc];
		dirp->dd_loc += sizeof(struct olddir);

		if ( dp->od_ino != 0 )	/* not deleted entry */
			{
			static struct direct	dir;	/* simulated */

			dir.d_ino = dp->od_ino;
			(void)strncpy( dir.d_name, dp->od_name, DIRSIZ
				     );
			dir.d_name[DIRSIZ] = '\0';
			dir.d_namlen = strlen( dir.d_name );
			dir.d_reclen = sizeof(struct direct)
				     - MAXNAMLEN + 3
				     + dir.d_namlen - dir.d_namlen % 4;
			return &dir;	/* -> simulated structure */
			}
		}
	}
