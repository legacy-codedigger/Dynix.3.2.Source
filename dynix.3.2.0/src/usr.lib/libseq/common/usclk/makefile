# Makefile to build microsecond clock sample programs.
# Usage: make [target]

c_usclk:
	cc -O -o c_usclk c_usclk.c -lseq

f_usclk:
	fortran -e -o f_usclk f_usclk.f -lseq

p_usclk:
	pascal -e -o p_usclk p_usclk.p -lseq

all: c_usclk f_usclk p_usclk

clean:
	rm -f c_usclk f_usclk p_usclk
