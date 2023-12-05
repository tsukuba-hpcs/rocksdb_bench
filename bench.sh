#!/bin/bash

set -eu

SDIR=/mnt/nvme0n1p1/bench
BIN=./posix_bench
rm -rf $SDIR
rm -rf log/*
mkdir -p $SDIR

#SDIR=/dev/dax0.0
#pmempool rm $SDIR
#pmempool create -l pmemkv obj $SDIR

MAXSIZE=$((1 * 1000 * 1000 * 1000)) # 1GB

#PMEMOBJ_CONF="prefault.at_open=1;prefault.at_create=1"
#export PMEMOBJ_CONF

vsize=512
while [ $vsize -le $((8*1024*1024)) ] # loop value size 1B to 8MB by 2x
do
	echo "vsize = $vsize B"
	for TH in 1 2 4 8 16 32 # loop thread size 1 to 8
	#for TH in 1 2 4 8
	do
		vmsize=$((vsize + 64))
		OUTF="log/out-${vmsize}-${TH}"
		
		# bench 10 keys
		N=10
		echo -n "$N start"
		[ $((N * vmsize)) -gt $MAXSIZE ] && N=$((MAXSIZE / vmsize)) # cast maxsize
		$BIN -v $vmsize -n $N -T $TH $SDIR > $OUTF.0
		set $(grep put $OUTF.0)
		TIME=$4
		echo " $TIME sec"
		
		# calc keys for around sec
		DURATION=10
		N=$(echo "scale=0; $DURATION * $N / $TIME" | bc -l)
		echo -n "$N (about 10 sec)"
		[ $((N * vmsize)) -gt $MAXSIZE ] && N=$((MAXSIZE / vmsize)) # cast maxsize
		echo -n " $N"
		$BIN -v $vmsize -n $N -T $TH $SDIR > $OUTF.1
		set $(grep put $OUTF.1)
		TIME=$4
		echo " $TIME sec"
		
		# calc keys for around sec
		#DURATION=10
		#N=$(echo "scale=0; $DURATION * $N / $TIME" | bc -l)
		#echo -n "$N (about 20 sec or 1GB)"
		#[ $((N * vmsize)) -gt $MAXSIZE ] && N=$((MAXSIZE / vmsize)) # cast maxsize
		#echo "$N"
		#$BIN -v $vmsize -n $N -T $TH $SDIR > $OUTF.2
	
	done
	vsize=$((vsize*32))
done
