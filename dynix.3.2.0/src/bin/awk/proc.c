/* $Copyright:	$
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

#ifndef lint
static char rcsid[] = "$Header: proc.c 2.0 86/01/28 $";
#endif

#include "awk.h"
#define NULL 0
struct xx
{	int token;
	char *name;
	char *pname;
} proc[] = {
	{ PROGRAM, "program", NULL},
	{ BOR, "boolop", " || "},
	{ AND, "boolop", " && "},
	{ NOT, "boolop", " !"},
	{ NE, "relop", " != "},
	{ EQ, "relop", " == "},
	{ LE, "relop", " <= "},
	{ LT, "relop", " < "},
	{ GE, "relop", " >= "},
	{ GT, "relop", " > "},
	{ ARRAY, "array", NULL},
	{ INDIRECT, "indirect", "$("},
	{ SUBSTR, "substr", "substr"},
	{ INDEX, "sindex", "sindex"},
	{ SPRINTF, "asprintf", "sprintf "},
	{ ADD, "arith", " + "},
	{ MINUS, "arith", " - "},
	{ MULT, "arith", " * "},
	{ DIVIDE, "arith", " / "},
	{ MOD, "arith", " % "},
	{ UMINUS, "arith", " -"},
	{ PREINCR, "incrdecr", "++"},
	{ POSTINCR, "incrdecr", "++"},
	{ PREDECR, "incrdecr", "--"},
	{ POSTDECR, "incrdecr", "--"},
	{ CAT, "cat", " "},
	{ PASTAT, "pastat", NULL},
	{ PASTAT2, "dopa2", NULL},
	{ MATCH, "matchop", " ~ "},
	{ NOTMATCH, "matchop", " !~ "},
	{ PRINTF, "aprintf", "printf"},
	{ PRINT, "print", "print"},
	{ SPLIT, "split", "split"},
	{ ASSIGN, "assign", " = "},
	{ ADDEQ, "assign", " += "},
	{ SUBEQ, "assign", " -= "},
	{ MULTEQ, "assign", " *= "},
	{ DIVEQ, "assign", " /= "},
	{ MODEQ, "assign", " %= "},
	{ IF, "ifstat", "if("},
	{ WHILE, "whilestat", "while("},
	{ FOR, "forstat", "for("},
	{ IN, "instat", "instat"},
	{ NEXT, "jump", "next"},
	{ EXIT, "jump", "exit"},
	{ BREAK, "jump", "break"},
	{ CONTINUE, "jump", "continue"},
	{ FNCN, "fncn", "fncn"},
	{ GETLINE, "getline", "getline"},
	{ 0, ""},
};
#define SIZE	LASTTOKEN - FIRSTTOKEN
char *table[SIZE];
char *names[SIZE];
main()
{	struct xx *p;
	int i;
	printf("#include \"awk.def\"\n");
	printf("obj nullproc();\n");
	for(p=proc;p->token!=0;p++)
		if(p==proc || strcmp(p->name, (p-1)->name))
			printf("extern obj %s();\n",p->name);
	for(p=proc;p->token!=0;p++)
		table[p->token-FIRSTTOKEN]=p->name;
	printf("obj (*proctab[%d])() = {\n", SIZE);
	for(i=0;i<SIZE;i++)
		if(table[i]==0) printf("/*%s*/\tnullproc,\n",tokname(i+FIRSTTOKEN));
		else printf("/*%s*/\t%s,\n",tokname(i+FIRSTTOKEN),table[i]);
	printf("};\n");
	printf("char *printname[%d] = {\n", SIZE);
	for(p=proc; p->token!=0; p++)
		names[p->token-FIRSTTOKEN] = p->pname;
	for(i=0; i<SIZE; i++)
		printf("/*%s*/\t\"%s\",\n",tokname(i+FIRSTTOKEN),
						names[i]?names[i]:"");
	printf("};\n");
	exit(0);
}