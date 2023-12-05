CFLAGS=-g -O2 -Wall -fopenmp `pkg-config --cflags rocksdb`
LDLIBS=`pkg-config --libs rocksdb`

SCRIPTS=bench.sh parse.sh

all: rocksdb_bench posix_bench

posix_bench: posix_bench.o fs_posix.o log.o file.o timespec.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
	
dist:
	rm -f rocksdb_bench.tar.gz
	tar zcf rocksdb_bench.tar.gz Makefile $(SRCS) $(SCRIPTS)

clean:
	rm -f rocksdb_bench posix_bench *.o
