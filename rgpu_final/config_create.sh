## This shell script creates the necessary configuration file
## when run in a node of a cluster that has access to every other node
## It also tries to recognize all the GPU resources available.
## Tested only on cslab@ntua cluster. The user should always check the final
## configuration file produced for errors!
## Note :  This script uses SSH to connect to the machines and obtain information about the GPUS
##	   SSH keys are required in all the machines for the script to run smoothly.


#!/bin/bash

queue -a | awk '{if (NR>3) print $1}' > nodes

id=0
for line in $(cat nodes)
do
	name=$line
	echo "Obtaining information about node \"$name\".."
	ip=`nslookup $name | awk '{if (NR==5) print $2}'`
	echo "Checking GPU configuration.."
	nr_gpus=`ssh $name 'nvidia-smi -L 2>/dev/null | grep GPU | wc -l'`
	if [ $? -eq 255 ]
	then
		continue
	fi
	echo "Found : $nr_gpus GPUs"
	echo "Done"
	echo $id $name $ip $nr_gpus >> nodes.config
	id=`expr $id + 1`

	echo "Complete"
done

rm -f nodes
