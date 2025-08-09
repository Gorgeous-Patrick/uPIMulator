rm -rf bin
mkdir bin
./build/uPIMulator --root_dirpath /home/patrickli/Space/uPIMulator/golang/uPIMulator --bin_dirpath /home/patrickli/Space/uPIMulator/golang/uPIMulator/bin --benchmark VA --num_channels 1 --num_ranks_per_channel 1 --num_dpus_per_rank 1 --num_tasklets 16 --data_prep_params 1024

