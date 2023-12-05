#!/bin/bash

ls plot/*-bw | \
	while read line
	do
		gnuplot -c "$line"
	done

ls *.eps | \
	while read line
	do
		convert -density 300 "$line" "$line.jpg"
		#rm -f $line
	done
