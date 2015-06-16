	for j in $(seq 1 3)
	do
		
		export LD_LIBRARY_PATH=/home/users/akarkat/rcuda/framework/rCUDAl/:$LD_LIBRARY_PATH
		export RGPU_HYPERV=termi3
		~/rgpu_final/rgpu_client -r ~/rcuda/benchs/bin/x86_64/linux/release/transpose > out_$j &
	sleep 2
	done
	

