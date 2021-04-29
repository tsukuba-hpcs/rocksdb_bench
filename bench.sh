SDIR=/mnt/pmem0/tatebe
mkdir -p $SDIR
rm $SDIR/kv.db

#SDIR=/dev/dax0.0
#pmempool rm $SDIR
#pmempool create -l pmemkv obj $SDIR

DBSIZE=340000000000
MAXSIZE=300000000000

#PMEMOBJ_CONF="prefault.at_open=1;prefault.at_create=1"
#export PMEMOBJ_CONF

vsize=1
while [ $vsize -le $((8*1024*1024)) ]
do
for TH in 1 2 3 4 5 6 7 8
do
vmsize=$((vsize + 64))
OUTF=out-$vmsize-$TH

N=10000
echo $N
[ $((N * vmsize)) -gt $MAXSIZE ] && N=$((MAXSIZE / vmsize))
echo $N
./pmemkv_bench -s $DBSIZE -v $vmsize -n $N -T $TH $SDIR > $OUTF.0
set $(grep put $OUTF.0)
TIME=$4

DURATION=10
N=$(echo "scale=0; $DURATION * $N / $TIME" | bc -l)
echo $N
[ $((N * vmsize)) -gt $MAXSIZE ] && N=$((MAXSIZE / vmsize))
echo $N
./pmemkv_bench -s $DBSIZE -v $vmsize -n $N -T $TH $SDIR > $OUTF.1
set $(grep put $OUTF.1)
TIME=$4

DURATION=40
N=$(echo "scale=0; $DURATION * $N / $TIME" | bc -l)
echo $N
[ $((N * vmsize)) -gt $MAXSIZE ] && N=$((MAXSIZE / vmsize))
echo $N
./pmemkv_bench -s $DBSIZE -v $vmsize -n $N -T $TH $SDIR > $OUTF.2

done
vsize=$((vsize*2))
done
