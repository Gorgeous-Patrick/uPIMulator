rm -rf bin
mkdir bin
pushd script || exit 1
python3 build.py || exit 1
popd || exit 1
./build/uPIMulator --root_dirpath /home/patrickli/Space/uPIMulator/golang/uPIMulator --bin_dirpath /home/patrickli/Space/uPIMulator/golang/uPIMulator/bin --benchmark BFS --num_channels 1 --num_ranks_per_channel 1 --num_dpus_per_rank 1 --num_tasklets 16 --data_prep_params 1024

