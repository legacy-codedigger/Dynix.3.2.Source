/* atob: version 4.0
 * stream filter to change printable ascii from "btoa" back into 8 bit bytes
 * if bad chars, or Csums do not match: exit(1) [and NO output]
 *
 *  Paul Rutter		Joe Orost
 *  philabs!per		petsd!joe
 */

#ifndef	lint
static char rcsid[] = "$Header: atob.c 2.1 90/05/09 $";
#endif

#include <stdio.h>

#define reg register

#define streq(s0, s1)  strcmp(s0, s1) == 0

#define times85(x)	((((((x<<2)+x)<<2)+x)<<2)+x)

long int Ceor = 0;
long int Csum = 0;
long int Crot = 0;
long int word = 0;
long int bcount = 0;

fatal(flg)
char flg;
{
	if (flg)
		fprintf(stderr, "atob: bad format or Csum\n");
	exit(1);
}

#define DE(c) ((c) - '!')

decode(c) 
  reg c;
{
  if (c == 'z') {
    if (bcount != 0) {
      fatal(1);
    } else {
      byteout(0);
      byteout(0);
      byteout(0);
      byteout(0);
    }
  } else if ((c >= '!') && (c < ('!' + 85))) {
    if (bcount == 0) {
      word = DE(c);
      ++bcount;
    } else if (bcount < 4) {
      word = times85(word);
      word += DE(c);
      ++bcount;
    } else {
      word = times85(word) + DE(c);
      byteout((int)((word >> 24) & 255));
      byteout((int)((word >> 16) & 255));
      byteout((int)((word >> 8) & 255));
      byteout((int)(word & 255));
      word = 0;
      bcount = 0;
    }
  } else {
    fatal(1);
  }
}

FILE *tmp_file;

byteout(c) 
  reg c;
{
  Ceor ^= c;
  Csum += c;
  Csum += 1;
  if ((Crot & 0x80000000)) {
    Crot <<= 1;
    Crot += 1;
  } else {
    Crot <<= 1;
  }
  Crot += c;
  putc(c, tmp_file);
}

main(argc, argv) 
  char **argv;
{
  reg c;
  reg long int i;
  char tmp_name[100];
  char buf[100];
  long int n1, n2, oeor, osum, orot;

  if (argc != 1) {
    fprintf(stderr,"bad args to %s\n", argv[0]);
    exit(2);
  }
  sprintf(tmp_name, "/usr/tmp/atob.%x", getpid());
  tmp_file = fopen(tmp_name, "w+");
  if (tmp_file == NULL) {
    fprintf(stderr, "atob: can't open temp file %s\n", tmp_name);
    fatal(0);
  }
  unlink(tmp_name);	/* Make file disappear */
  /*search for header line*/
  for (;;) {
    if (fgets(buf, sizeof buf, stdin) == NULL) {
      fatal(1);
    }
    if (streq(buf, "xbtoa Begin\n")) {
      break;
    }
  }

  while ((c = getchar()) != EOF) {
    if (c == '\n') {
      continue;
    } else if (c == 'x') {
      break;
    } else {
      decode(c);
    }
  }
  if(scanf("btoa End N %ld %lx E %lx S %lx R %lx\n",
         &n1, &n2, &oeor, &osum, &orot) != 5) {
    fatal(1);
  }
  if ((n1 != n2) || (oeor != Ceor) || (osum != Csum) || (orot != Crot)) {
    fatal(1);
  } else {
    /*copy OK tmp file to stdout*/;
    fseek(tmp_file, 0L, 0);
    for (i = n1; --i >= 0;) {
      putchar(getc(tmp_file));
    }
  }
  exit(0);
}
