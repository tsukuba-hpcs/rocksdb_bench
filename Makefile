#CFLAGS=-g -O2 -Wall -fsanitize=address -fno-omit-frame-pointer -fopenmp
CFLAGS=-g -O2 -Wall -fopenmp
SCRIPTS=bench.sh parse.sh
RDB=`pkg-config --cflags rocksdb`
RLB=`pkg-config --libs rocksdb`

all: rocksdb_bench posix_bench

posix_bench: posix_bench.o fs_posix.o log.o file.o timespec.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

rocksdb_bench: rocksdb_bench.c
	$(CC) $(CFLAGS) $(RDB) -o $@ $^ $(LDLIBS) $(RLB)
	
dist:
	rm -f rocksdb_bench.tar.gz
	tar zcf rocksdb_bench.tar.gz Makefile $(SRCS) $(SCRIPTS)

clean:
	rm -f rocksdb_bench posix_bench *.o
