i=1
while [ $i -le $((8*1024*1024)) ]
do
for p in 1 2 3 4 5 6 7 8
do
for l in put get remove
do
ii=$((i + 64))
echo $ii $(grep $l out-$ii-$p.2 | awk '{ print $5 }') >> $l-$p-bw.dat
echo $ii $(grep $l out-$ii-$p.2 | awk '{ print $6 }') >> $l-$p-iops.dat
done
done
i=$((i*2))
done
