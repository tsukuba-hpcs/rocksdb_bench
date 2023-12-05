#!/bin/bash

rm -f log/*.dat

i=1
while [ $i -le $((8*1024*1024)) ]
do
	for p in 1 2 4 8 16 32
	do
		for l in put get remove
		do
			ii=$((i + 64))
			[ ! -e "log/out-$ii-$p.1" ] && break
			echo $ii $(grep $l log/out-$ii-$p.1 | awk '{ print $5 }') >> log/$l-$p-bw.dat
			echo $ii $(grep $l log/out-$ii-$p.1 | awk '{ print $6 }') >> log/$l-$p-iops.dat
		done
	done
	i=$((i*2))
done
