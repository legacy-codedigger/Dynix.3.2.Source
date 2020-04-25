#
# Makefile for example parallel programs from the
# "Guide to Parallel Programming"
#
# Targets for compiling the programs
#

all: & cstatic pstatic cdynamic pdynamic shmem

cstatic: cstatic.c
	cc cstatic.c -lpps -o cstatic

pstatic: pstatic.pas
	pascal -mp pstatic.pas -o pstatic

cdynamic: cdynamic.c
	cc cdynamic.c -lpps -lm -o cdynamic

pdynamic: pdynamic.pas
	pascal -mp pdynamic.pas -o pdynamic

shmem: shmem.c
	cc shmem.c -lpps -o shmem
#
# Clean-up
#

clean:
	rm -f cstatic pstatic cdynamic pdynamic shmem
