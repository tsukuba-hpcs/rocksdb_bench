CFLAGS=-g -O -Wall -fopenmp `pkg-config --cflags rocksdb`
LDLIBS=`pkg-config --libs rocksdb`

SRCS=rocksdb_bench.c kv_err.c
SCRIPTS=bench.sh parse.sh

all: rocksdb_bench

dist:
	rm -f rocksdb_bench.tar.gz
	tar zcf rocksdb_bench.tar.gz Makefile $(SRCS) $(SCRIPTS)
