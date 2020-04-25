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

/* $Header: stabstring.h 1.5 1991/06/07 20:58:32 $ */

/*
 * Symbol classes
 */

typedef enum {
    BADUSE, CONST, TYPE, VAR, ARRAY, DYNARRAY, SUBARRAY, PTRFILE, RECORD, FIELD,
    PROC, FUNC, FVAR, REF, PTR, FILET, SET, RANGE, 
    LABEL, WITHPTR, SCAL, STR, PROG, IMPROPER, VARNT,
    FPROC, FFUNC, MODULE, TAG, COMMON, EXTREF, TYPEREF, ENTRY
} Symclass;

typedef enum { R_CONST, R_TEMP, R_ARG, R_ADJUST, R_ASSUMED } Rangetype; 

struct symbol {
	char		*name;
	Symclass	class;
	struct	symbol *type;
	struct	symbol *chain;
	union {
		int	constval;	/* value of constant symbol */
		int	offset;		/* variable address */
		long	iconval;	/* integer constant value */
		int	ndims;		/* dimensions for dynamic/sub-arrays */
		struct {		/* field offset and size (in bits) */
			int	offset;
			int	length;
		} field;
		struct {		/* range bounds */
			Rangetype lowertype; 
			Rangetype uppertype;  
			long	lower;
			long	upper;
		} rangev;
		struct {
			long	beginaddr;	/* address of function code */
		} funcv;
		struct {		/* variant record info */
			int	size;
			struct	symbol * vtorec;
			struct	symbol * vtag;
		} varnt;
		char	*typeref;	/* type defined by "<module>:<type>" */
		struct	symbol * extref;/* indirect symbol for external reference */
	} symvalue;
	struct symbol * next_sym;	/* hash chain */
	struct nlist	* np;		/* XXXX for old style STAB support */
	struct sdb	* sdb;		/* pointer to sdb entry */
};


struct symbol * t_boolean;
struct symbol * t_char;
struct symbol * t_int;
struct symbol * t_real;
struct symbol * t_nil;
struct symbol * t_addr;

