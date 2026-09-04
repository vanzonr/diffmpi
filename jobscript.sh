#!/bin/bash
#SBATCH -N 1
#SBATCH -n 192
#SBATCH --time 12:00:00
#SBATCH --output out_%j.txt

module load StdEnv/2023 gcc/12.3 openmpi/4.1.5 boost

make

export OMP_NUM_THREADS=1

source /scinet/vast/etc/vastpreload-openmpi.bash # important if doing MPI-IO
mkdir 1 2 3 4 # each case in its own directory

# assume the input files called 1.in, 2.in, 3.in, 4.in
# run 4 instances each in their own directory

parallel -j 4 'cd {} ; mpirun -np 48 ../diff2d ../diff2dhuge.ini' ::: 1 2 3 4
