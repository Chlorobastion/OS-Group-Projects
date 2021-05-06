#!/bin/sh
module load foss
let cores=$1
let m=(0)
for x in 1000000 500000 100000 50000 10000
do
let m=(m+1)
for y in {1..4}
do
mpirun -n $cores mpi $x > mpi-"$cores"-"$m"-"$y".out
done
done
exit 0