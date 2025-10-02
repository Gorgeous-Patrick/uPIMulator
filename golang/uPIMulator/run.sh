#!/bin/bash
rm -rf bin
mkdir bin
# pushd script || exit 1
# python3 build.py || exit 1
# popd || exit 1
./build/uPIMulator --root_dirpath $PWD --bin_dirpath $PWD/bin --benchmark GEN --num_channels 1 --num_ranks_per_channel 1 --num_dpus_per_rank 1 --num_tasklets $NUM_TASKLETS --data_prep_params 1000

