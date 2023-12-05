#!/bin/bash

ls plot | \
	while read line
	do
		gnuplot -c "plot/$line"
	done

ls *.eps | \
	while read line
	do
		convert -density 300 "$line" "$line.jpg"
		#rm -f $line
	done
