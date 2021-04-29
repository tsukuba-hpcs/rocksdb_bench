CFLAGS=-O -Wall -fopenmp
LDFLAGS=-lpmemkv

SRCS=pmemkv_bench.c
SCRIPTS=bench.sh parse.sh

all: pmemkv_bench

dist:
	rm -f pmemkv_bench.tar.gz
	tar zcf pmemkv_bench.tar.gz Makefile $(SRCS) $(SCRIPTS)
