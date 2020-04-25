/*
	testdir -- test for C library directory access extensions

	last edit:	09-Jul-1983	D A Gwyn
*/

#include	<dir.h>
#include	<stdio.h>

extern void	exit();
extern int	strcmp(), strlen();

main( argc, argv )
	int			argc;
	char			**argv;
	{
	register DIR		*dirp;
	register struct direct	*dp;
	int			nerrs = 0;	/* total not found */

	if ( (dirp = opendir( "." )) == NULL )
		{
		(void)fprintf( stderr, "Cannot open \".\" directory\n"
			     );
		exit( 1 );
		}

	while ( --argc > 0 )
		{
		register int	len = strlen( *++argv );

		while ( (dp = readdir( dirp)) != NULL )
			if ( dp->d_namlen == len
			  && strcmp( dp->d_name, *argv ) == 0
			   )	{
				(void)printf( "\"%s\" found.\n",
					      *argv
					    );
				break;
				}
		if ( dp == NULL )
			{
			(void)printf( "\"%s\" not found.\n", *argv );
			++nerrs;
			}

		rewinddir( dirp );
		}

	closedir( dirp );
	exit( nerrs );
	}
