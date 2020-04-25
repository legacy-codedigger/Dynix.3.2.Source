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

#include "crash.h"
#include <a.out.h>
#include <stab.h>
#include <strings.h>
#include "stabstring.h"

/*
 * Special characters in symbol table information.
 */

#define CONSTNAME 'c'
#define TYPENAME 't'
#define TAGNAME 'T'
#define MODULEBEGIN 'm'
#define EXTPROCEDURE 'P'
#define PRIVPROCEDURE 'Q'
#define INTPROCEDURE 'I'
#define EXTFUNCTION 'F'
#define PRIVFUNCTION 'f'
#define INTFUNCTION 'J'
#define EXTVAR 'G'
#define MODULEVAR 'S'
#define OWNVAR 'V'
#define REGVAR 'r'
#define VALUEPARAM 'p'
#define VARIABLEPARAM 'v'
#define LOCALVAR /* default */

/*
 * Type information special characters.
 */

#define T_SUBRANGE 'r'
#define T_ARRAY 'a'
#define T_OLDOPENARRAY 'A'
#define T_OPENARRAY 'O'
#define T_DYNARRAY 'D'
#define T_SUBARRAY 'E'
#define T_RECORD 's'
#define T_UNION 'u'
#define T_ENUM 'e'
#define T_PTR '*'
#define T_FUNCVAR 'f'
#define T_PROCVAR 'p'
#define T_IMPORTED 'i'
#define T_SET 'S'
#define T_OPAQUE 'o'
#define T_FILE 'd'

#define	NTYPES	1000
#define HASHTABLESIZE 50

struct symbol *typetable[NTYPES];
struct symbol *symtab[HASHTABLESIZE];
char *curchar;
struct symbol *constype();

panic(s, a, b, c, d, e, f)
{
	printf(s, a, b, c, d, e, f);
	printf("\n");
	exit(1);
}

static char *
identname(n, tf)
	char *n;
{
	/* possibly store data in separate struct, depending upon tf */
	return(n);
}

static char *
getcont()
{
	panic("continuation character");
	return(0);
}

#define skipchar(ptr, ch) \
{ \
    if (*ptr != ch) { \
	panic("expected char '%c', found '%s'", ch, ptr); \
    } \
    ++ptr; \
}

#define optchar(ptr, ch) \
{ \
    if (*ptr == ch) { \
	++ptr; \
    	} \
}

#define chkcont(ptr) \
{ \
    if (*ptr == '?') { \
	ptr = getcont(); \
    	} \
}

#define newSym(s, n) \
{ \
    s = insert(n); \
}

struct symbol *
symbol_alloc()
{
	static struct symbol *s = (struct symbol *)0;
	static int s_left = 0;

	if (s_left != 0) {
		s_left--;
		return(s++);
	}

	if ((s = (struct symbol *) malloc(sizeof(struct symbol) * NTYPES)) 
	    == NULL) {
		panic("can't allocate space for symbol table entries");
	}
	bzero((char *)s, sizeof(struct symbol) * NTYPES);
	s_left = NTYPES - 1;
	return(s++);
}

static struct symbol *
newSymbol(name, class, type, chain)
	char * name;
	Symclass class;
	struct symbol *type;
	struct symbol *chain;
{
	struct symbol *s;

	s = symbol_alloc();
	s->name = name;
	s->class = class;
	s->type = type;
	s->chain = chain;
	s->sdb = 0;
	return s;
}

/*
 * Insert a symbol into the hash table.
 */

static
hash(s)
	char *s;
{
	int h;
	char *p;

	h = 0;
    	for (p = s; *p != '\0'; p++) {
        	h = (h << 1) ^ (*p);
    	}
    	return(h % HASHTABLESIZE);


}

static struct symbol *
insert(name)
	char *name;
{
	struct symbol *s;
	int h;

	h = hash(name);
	s = symbol_alloc();
	s->name = name;
	s->next_sym = symtab[h];
	symtab[h] = s;
	return s;
}

/*
 * Symbol lookup.
 */

struct symbol *
lookup(name)
	char *name;
{
	struct symbol *s = NULL;
	int h;

	if (symtab) {
		h = hash(name);
		s = symtab[h];
		while (s != NULL && strcmp(s->name, name) != 0) {
			s = s->next_sym;
		}
	}
	return s;
}


int
search_stag(name)
	char *name;
{
	int val;
	struct sdb *p;
	struct symbol *s;
	struct symbol *t;

	err_search = 0;
	s = lookup(name);
	if (s == NULL){
		/*
		 * Old format: STAGs should be N_GSYM but might be
		 * N_SSYM.
		 */
		if (lookbysym(name, &val, N_GSYM)) { 
			sdbinfo.sdb_desc &= PCCTM_BASETYPE;
			return (1);
		}
		if (lookbysym(name, &val, N_SSYM)) {
			sdbinfo.sdb_desc &= PCCTM_BASETYPE;
			return (1);
		}
		err_search = 1;
		if (debug[18])
			printf("search_stag('%s') failed\n", name);
		if (debug[15])
			printf("seatch_stag('%s') old style stag\n", name);
		return (0);
	}
	t = s->type;
	if (debug[15]) {
		printf("search_stag('%s') new style stag @0x%x type=0x%x\n",
			name, s, t);
	}
	if (t->sdb) {
		sdbinfo = *(t->sdb);
	} else {
		bzero(&sdbinfo, sizeof(sdbinfo));
		sdbinfo.sdb_name = s->name;
		sdbinfo.sdb_size = size(s);
		sdbinfo.sdb_sym = s;
	}
	return (1);
}


struct symbol *
entersym (name, np)
	char * name;
	struct nlist *np;
{
	struct symbol *s;
	struct symbol *ds;
	char *p;
	char * n;
	char c;
	char *common_name, *end_common_name;
	int in_common;
	int is_ref;
	struct symbol *extVar();


	p = index(name, ':');
	*p = '\0';
	c = *(p+1);

	curchar = p + 2;

	switch (c) {
	case TAGNAME:
		s = symbol_alloc();
		s->name = name;
		s->np = np;		/* XXXX old style STAB support */
		tagName(s);
		break;
	
	case TYPENAME:
		newSym(s, name);
		typeName(s);
		break;

	case EXTVAR:
	    	s = extVar(name, np->n_value);
		s->np = np;		/* XXXX old style STAB support */
	    	break;

	case EXTFUNCTION:
		/*
		 * Not realy useful since we can not do anything with the
		 * values (asm functions).
		 */
#ifdef NOTDEF
		if ((s = lookup()) == NULL) {
			newSym(s, name);
			s->class = EXTREF;
			sym_add(np);	/* will recurs and call entersym() */
		}
#endif
		return NULL;

	default:
		printf("entersym: '%s' not entered type=%d\n",name,c);
		return NULL;
	}

	return(s);
}

static
typeName (s)
	struct symbol *s;
{
	int i;

	s->class = TYPE;
	i = getint();
	if (i == 0) {
		panic("bad input on type \"%s\" at \"%s\"", s, curchar);
	} 
	else if (i >= NTYPES) {
		panic("too many types");
	}
	/*
	 * A hack for C typedefs that don't create new types,
	 * e.g. typedef unsigned int Hashvalue;
	 *  or  typedef struct blah BLAH;
	 */
	if (*curchar != '=') {
		s->type = typetable[i];
		if (s->type == NULL) {
			s->type = symbol_alloc();
			typetable[i] = s->type;
		}
	} else {
		if (typetable[i] != NULL) {
			typetable[i]->class = TYPE;
			typetable[i]->type = s;
		} 
		else {
			typetable[i] = s;
		}
		skipchar(curchar, '=');
		getType(s);
	}
}

/*
 * Enter a tag name.
 */

static
tagName (s)
	struct symbol *s;
{
	int i;

	s->class = TAG;
	i = getint();
	if (i == 0) {
		panic("bad input on tag \"%s\" at \"%s\"",s->name, curchar);
	} else if (i >= NTYPES) {
		panic("too many types");
	}
	if (typetable[i] != NULL) {
		typetable[i]->class = TYPE;
		typetable[i]->type = s;
	} else {
		typetable[i] = s;
	}
	skipchar(curchar, '=');
	getType(s);
}


/*
 * Get a type from the current stab string for the given symbol.
 */

static
getType (s)
	struct symbol *s;
{
	s->type = constype(NULL);
	if (s->class == TAG) {
		addtag(s);
	}
}

/*
 * Construct a type out of a string encoding.
 */

Rangetype getRangeBoundType();

static struct symbol *
constype (type)
	struct symbol *type;
{
	struct symbol *t;
	int n;
	char class;
	char *p;

	while (*curchar == '@') {
		p = index(curchar, ';');
		if (p == NULL) {
			fflush(stdout);
			fprintf(stderr, "missing ';' after type attributes");
		} else {
			curchar = p + 1;
		}
	}
	if (isdigit(*curchar)) {
		n = getint();
		if (n >= NTYPES) {
			panic("too many types");
		}
		if (*curchar == '=') {
			if (typetable[n] != NULL) {
				t = typetable[n];
			} else {
				t = symbol_alloc();
				typetable[n] = t;
			}
			++curchar;
			constype(t);
		} else {
			t = typetable[n];
			if (t == NULL) {
				t = symbol_alloc();
				typetable[n] = t;
			}
		}
	} else {
		if (type == NULL) {
			t = symbol_alloc();
		} else {
			t = type;
		}
		class = *curchar++;
		switch (class) {
		case T_SUBRANGE:
			consSubrange(t);
			break;

		case T_ARRAY:
			t->class = ARRAY;
			t->chain = constype(NULL);
			skipchar(curchar, ';');
			chkcont(curchar);
			t->type = constype(NULL);
			break;

		case T_OLDOPENARRAY:
			t->class = DYNARRAY;
			t->symvalue.ndims = 1;
			t->type = constype(NULL);
			t->chain = t_int;
			break;

		case T_OPENARRAY:
		case T_DYNARRAY:
			consDynarray(t);
			break;

		case T_SUBARRAY:
			t->class = SUBARRAY;
			t->symvalue.ndims = getint();
			skipchar(curchar, ',');
			t->type = constype(NULL);
			t->chain = t_int;
			break;

		case T_RECORD:
			consRecord(t, RECORD);
			break;

		case T_UNION:
			consRecord(t, VARNT);
			break;

		case T_ENUM:
			consEnum(t);
			break;

		case T_PTR:
			t->class = PTR;
			t->type = constype(NULL);
			break;

			/*
				     * C function variables are different from Modula-2's.
				     */
		case T_FUNCVAR:
			t->class = FFUNC;
			t->type = constype(NULL);
			break;

		case T_PROCVAR:
			t->class = FPROC;
			consParamlist(t);
			break;

		case T_IMPORTED:
			consImpType(t);
			break;

		case T_SET:
			t->class = SET;
			t->type = constype(NULL);
			break;

		case T_OPAQUE:
			consOpaqType(t);
			break;

		case T_FILE:
			t->class = FILET;
			t->type = constype(NULL);
			break;

		default:
			panic("bogus type found: \"%c\"\n", class);
		}
	}
	return t;
}

/*
 * Construct a subrange type.
 */

static
consSubrange (t)
	struct symbol *t;
{
	t->class = RANGE;
	t->type = constype(NULL);
	t->name = t->type->name;	/* put the type name in the RANGE symbol too */
	skipchar(curchar, ';');
	chkcont(curchar);
	t->symvalue.rangev.lowertype = getRangeBoundType();
	t->symvalue.rangev.lower = getint();
	skipchar(curchar, ';');
	chkcont(curchar);
	t->symvalue.rangev.uppertype = getRangeBoundType();
	t->symvalue.rangev.upper = getint();
}

/*
 * Figure out the bound type of a range.
 *
 * Some letters indicate a dynamic bound, ie what follows
 * is the offset from the fp which contains the bound; this will
 * need a different encoding when pc a['A'..'Z'] is
 * added; J is a special flag to handle fortran a(*) bounds
 */

static Rangetype 
getRangeBoundType ()
{
	Rangetype r;

	switch (*curchar) {
	case 'A':
		r = R_ARG;
		curchar++;
		break;

	case 'T':
		r = R_TEMP;
		curchar++;
		break;

	case 'J': 
		/*
		 * Code to handle SVS R_ASSUMED is already in place,
		 * so use it instead of R_ADJUST for ATP as well.
		 */
		/*r = R_ADJUST;*/
		r = R_ASSUMED;
		curchar++;
		break;

	default:
		r = R_CONST;
		break;
	}
	return r;
}

/*
 * Construct a dynamic array descriptor.
 */

static
consDynarray (t)
	struct symbol *t;
{
	t->class = DYNARRAY;
	t->symvalue.ndims = getint();
	skipchar(curchar, ',');
	t->type = constype(NULL);
	t->chain = t_int;
}

/*
 * Construct a record or union type.
 */

static
consRecord (t, class)
	struct symbol *t;
	Symclass class;
{
	struct symbol *u;
	char *cur, *p;
	char * name;

	t->class = class;
	t->symvalue.offset = getint();
	u = t;
	cur = curchar;
	while (*cur != ';' && *cur != '\0') {
		p = index(cur, ':');
		if (p == NULL) {
			panic("index(\"%s\", ':') failed", curchar);
		}
		*p = '\0';
		name = identname(cur, 1);
		u->chain = newSymbol(name, FIELD, NULL, NULL);
		cur = p + 1;
		u = u->chain;
		curchar = cur;
		u->type = constype(NULL);
		skipchar(curchar, ',');
		u->symvalue.field.offset = getint();
		skipchar(curchar, ',');
		u->symvalue.field.length = getint();
		skipchar(curchar, ';');
		chkcont(curchar);
		cur = curchar;
		sdb_add_ss(u);
	}
	if (*cur == ';') {
		++cur;
	}
	curchar = cur;
}

/*
 * Construct an enumeration type.
 */

static
consEnum (t)
	struct symbol *t;
{
	struct symbol *u;
	char *p;
	int count;

	t->class = SCAL;
	count = 0;
	u = t;
	while (*curchar != ';' && *curchar != '\0') {
		p = index(curchar, ':');
		*p = '\0';
		u->chain = insert(identname(curchar, 1));
		curchar = p + 1;
		u = u->chain;
		u->class = CONST;
		u->type = t;
		u->symvalue.constval = getint();
		++count;
		skipchar(curchar, ',');
		chkcont(curchar);
	}
	if (*curchar == ';') {
		++curchar;
	}
	t->symvalue.iconval = count;
}

/*
 * Construct a parameter list for a function or procedure variable.
 */

static
consParamlist (t)
	struct symbol *t;
{
	struct symbol *p;
	int i, n, paramclass;

	n = getint();
	skipchar(curchar, ';');
	p = t;
	for (i = 0; i < n; i++) {
		p->chain = newSymbol(NULL, VAR, NULL, NULL);
		p = p->chain;
		p->type = constype(NULL);
		skipchar(curchar, ',');
		paramclass = getint();
		if (paramclass == 0) {
			p->class = REF;
		}
		skipchar(curchar, ';');
		chkcont(curchar);
	}
}

/*
 * Construct an imported type.
 * Add it to a list of symbols to get fixed up.
 */

static
consImpType (t)
	struct symbol *t;
{
	char *p;

	p = curchar;
	while (*p != ',' && *p != ';' && *p != '\0') {
		++p;
	}
	if (*p == '\0') {
		panic("bad import symbol entry '%s'", curchar);
	}
	t->class = TYPEREF;
	t->symvalue.typeref = curchar;
	if (*p == ',') {
		curchar = p + 1;
		(void) constype(NULL);
	} else {
		curchar = p;
	}
	skipchar(curchar, ';');
	*p = '\0';
}

/*
 * Construct an opaque type entry.
 */

static
consOpaqType (t)
	struct symbol *t;
{
	char *p;
	struct symbol *s;
	char * n;
	int def;

	p = curchar;
	while (*p != ';' && *p != ',') {
		if (*p == '\0') {
			panic("bad opaque symbol entry '%s'", curchar);
		}
		++p;
	}
	def = (int) (*p == ',');
	*p = '\0';
	n = identname(curchar, 1);

	s = lookup(n);
    	while (s != NULL && !(s->name == n) && s->class == TYPEREF)
		s = s->next_sym;

	if (s == NULL) {
		s = insert(n);
		s->class = TYPEREF;
		s->type = NULL;
	}
	curchar = p + 1;
	if (def) {
		s->type = constype(NULL);
		skipchar(curchar, ';');
	}
	t->class = TYPE;
	t->type = s;
}

/*
 * Read an int from the current position in the type string.
 */

static int 
getint ()
{
	int n;
	char *p;
	int isneg;

	n = 0;
	p = curchar;
	if (*p == '-') {
		isneg = 1;
		++p;
	} else {
		isneg = 0;
	}
	while (isdigit(*p)) {
		n = 10*n + (*p - '0');
		++p;
	}
	curchar = p;
	return isneg ? (-n) : n;
}

/*
 * Add a tag name.  This is a kludge to be able to refer
 * to tags that have the same name as some other symbol
 * in the same block.
 */

static
addtag (s)
	struct symbol *s;
{
	struct symbol *t;

	t = insert(identname(s->name, 0));
	t->class = TAG;
	t->type = s->type;
}

static char *
showspecial1(c)
char c;
{
	switch (c) {
	case TYPENAME:
		return("TYPENAME");
	case TAGNAME:
		return("TAGNAME");
	default:
		return("default: LOCALVAR");
	}
}

static char *
showspecial2(c)
	Symclass c;
{
	switch (c) {
	case BADUSE:
		return("BADUSE");
	case CONST:
		return("CONST");
	case TYPE:
		return("TYPE");
	case VAR:
		return("VAR");
	case ARRAY:
		return("ARRAY");
	case DYNARRAY:
		return("DYNARRAY");
	case SUBARRAY:
		return("SUBARRAY");
	case PTRFILE:
		return("PTRFILE");
	case RECORD:
		return("RECORD");
	case FIELD:
		return("FIELD");
	case PROC:
		return("PROC");
	case FUNC:
		return("FUNC");
	case FVAR:
		return("FVAR");
	case REF:
		return("REF");
	case PTR:
		return("PTR");
	case FILET:
		return("FILET");
	case SET:
		return("SET");
	case RANGE:
		return("RANGE");
	case LABEL:
		return("LABEL");
	case WITHPTR:
		return("WITHPTR");
	case SCAL:
		return("SCAL");
	case STR:
		return("STR");
	case PROG:
		return("PROG");
	case IMPROPER:
		return("IMPROPER");
	case VARNT:
		return("VARNT");
	case FPROC:
		return("FPROC");
	case FFUNC:
		return("FFUNC");
	case MODULE:
		return("MODULE");
	case TAG:
		return("TAG");
	case COMMON:
		return("COMMON");
	case EXTREF:
		return("EXTREF");
	case TYPEREF:
		return("TYPEREF");
	case ENTRY:
		return("ENTRY");
	default:
		return("???");
	}
}

static
stab_dump()
{
	int i;
	struct symbol *s;

	for (i = 0; i < HASHTABLESIZE; i++) {
		for (s = symtab[i]; s; s = s->next_sym) {
			printf("found %s %s", showspecial2(s->class), s->name);
			if (s->class == TAG)
				tag_dump(s->type);
			if (s->class == TYPE)
				type_dump(s->type);
			printf("\n");
		}
	}
}

static
type_dump(t)
	struct symbol *t;
{
	struct symbol *s;

	printf(", a %s (%d)", showspecial2(t->class), t->symvalue.offset);
	if (t->class == RECORD || t->class == TAG) {
		printf(" which is");
		tag_dump(t);
		return;
	}

	printf("%s", t->name);
	return;
}

static
tag_dump(t)
	struct symbol *t;
{
	struct symbol *s;

	printf(" a %s", showspecial2(t->class));
	if (t->class == RECORD) {
		printf(" (%d) containing: ", t->symvalue.offset);
		t = t->chain;
		while (t) {
			printf("%s (%s) ", t->name, showspecial2(t->class));
			if (t->class == FIELD) {
				if (t->type->class == TYPE || 
				    t->type->class == TAG)
					printf("(%s %d) ", t->type->name,
					    t->symvalue.offset);
				else
					printf("(%s) ",
						showspecial2(t->type->class));
			}
			t = t->chain;
		}
	}
}

static struct symbol *
maketype(name, lower, upper)
	char * name;
	long lower;
	long upper;
{
	struct symbol * s;
	char * n;

	if (name == NULL) {
		n = NULL;
	} 
	else {
		n = identname(name, 1);
	}
	s = insert(n);
	s->class = TYPE;
	s->type = NULL;
	s->chain = NULL;
	s->type = newSymbol(NULL, 0, RANGE, s, NULL);
	s->type->symvalue.rangev.lower = lower;
	s->type->symvalue.rangev.upper = upper;
	return s;
}

/*
 * Create the builtin symbols.
 */

symbols_init ()
{
	struct symbol * s;

	if (t_boolean == NULL) {
		t_boolean = maketype("$boolean", 0L, 1L);
		t_int = maketype("$integer", 0x80000000L, 0x7fffffffL);
		t_char = maketype("$char", 0L, 255L);
		t_real = maketype("$real", 8L, 0L);
		t_nil = maketype("$nil", 0L, 0L);
		t_addr = insert(identname("$address", 1));
		t_addr->class = TYPE;
		t_addr->type = newSymbol(NULL, 1, PTR, t_int, NULL);
	}

	s = insert(identname("true", 1));
	s->class = CONST;
	s->type = t_boolean;
	s->symvalue.constval = 1;
	s = insert(identname("false", 1));
	s->class = CONST;
	s->type = t_boolean;
	s->symvalue.constval = 0;
}

/*
 * Resolve an "abstract" type reference.
 *
 * It is possible in C to define a pointer to a type, but never define
 * the type in a particular source file.  Here we try to resolve
 * the type definition.  This is problematic, it is possible to
 * have multiple, different definitions for the same name type.
 */

findtype(s)
	struct symbol * s;
{
	register struct symbol * t, *u, *prev;

	u = s;
	prev = NULL;
	while (u != NULL && u->class != BADUSE) {
		if (u->name != NULL) {
			prev = u;
		}
		u = u->type;
	}
	if (prev == NULL) {
		panic("couldn't find link to type reference");
	}
	t = lookup(prev->name);
	if (t == NULL) {
		panic("couldn't resolve reference");
	} else {
		prev->type = t->type;
	}
}


static struct symbol *
extVar (n, off)
	char * n;
	int off;
{
	struct symbol * s;

	s = lookup(n);
	while (s != NULL && !(strcmp(s->name, n) == 0 && s->class == VAR)) {
		s = s->next_sym;
	}
	if (s == NULL) {
		/* newSym(s, n) */	/* XXX puts it into hash table */
		s = symbol_alloc();
		s->name = n;
		s->class = VAR;
		s->symvalue.offset = off;
		getType(s);
		getExtRef(s);
	} 
	else {
		(void) constype(NULL);
	}
	return(s);
}

static
getExtRef (s)
	struct symbol * s;
{
	char *p;
	char * n;
	struct symbol * t;

	if (*curchar == ',' && *(curchar + 1) != '\0') {
		p = index(curchar + 1, ',');
		*curchar = '\0';
		if (p != NULL) {
			*p = '\0';
			n = identname(curchar + 1, 0);
			curchar = p + 1;
		} 
		else {
			n = identname(curchar + 1, 1);
		}
		t = insert(n);
		t->class = EXTREF;
		t->symvalue.extref = s;
	}
}

static
dump_symtab()
{
	int i;
	struct symbol *s;

	for (i = 0; i < HASHTABLESIZE; i++) {
		s = symtab[i];
		while (s != NULL) {
			printf("%s at %x", s->name, s);
			if (s->type)
				printf("type %d", s->type->class);
			printf("\n");
			s = s->next_sym;
		}
	}
}

find_field(s, f)
	struct symbol *s;	/* structure definition */
	char *f;		/* field name within */
{
	if (s == NULL)
		return(-1);

	switch (s->class) {
	case CONST:
	case VAR:
		return(-1);

	case TYPE:
	case TAG:
		return find_field(s->type, f);

	case RECORD:
	case VARNT:
		return find_field(s->chain, f);

	case FIELD:
		if (strcmp(f, s->name) == 0) {
			/*
			 * Set up the sdb field for this field but
			 * set up its sym field to be that this fields
			 * type, so that further references are to the new
			 * type.
			 */
			if (s->sdb) {
				sdbinfo = *(s->sdb);
			} else {
				bzero(&sdbinfo, sizeof(sdbinfo));
				sdbinfo.sdb_name = s->name;
				sdbinfo.sdb_size = size(s);
			}
			sdbinfo.sdb_sym = s->type->chain;
			return(s->symvalue.field.offset / 8); /* convert->byte*/
		} else
			return find_field(s->chain, f);

	default:
		printf("bogus type!\n");
		return(-1);
	}
}
