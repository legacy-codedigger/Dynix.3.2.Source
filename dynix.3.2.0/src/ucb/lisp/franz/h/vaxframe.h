/*					-[Sat Jan 29 14:02:34 1983 by jkf]-
 * 	vaxframe.h			$Locker:  $
 * vax calling frame definition
 *
 * $Header: vaxframe.h 1.1 86/05/20 $
 *
 * (c) copyright 1982, Regents of the University of California
 */

struct machframe {
	lispval	(*handler)();
	long	mask;
	lispval	*ap;
struct 	machframe	*fp;
	lispval	(*pc)();
	lispval	*r6;
	lispval *r7;
};
