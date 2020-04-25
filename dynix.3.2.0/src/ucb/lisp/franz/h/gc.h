/*					-[Sat Jan 29 13:56:06 1983 by jkf]-
 * 	gc.h			$Locker:  $
 * garbage collector metering definitions
 *
 * $Header: gc.h 1.1 86/05/20 $
 *
 * (c) copyright 1982, Regents of the University of California
 */
 
struct gchead
	{  int version;	/* version number of this dump file */
	   int lowdata;	/* low address of sharable lisp data */
	   int dummy,dummy2,dummy3; 	/* to be used later	*/
	};

