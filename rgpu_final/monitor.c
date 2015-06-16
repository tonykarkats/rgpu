#include <stdio.h>
#include <string.h>
#include "rgpu.h"

extern int monitor_mode;

/* This function monitors the current state of the cluster
   and returns the statistics in the nodeinfo structs passed
   as arguments. It can monitor in 2 modes AVG using an average
   of some last measurements or NOW that only keeps the last measurement */

void * monitor(gpuinfo *gpu[]) {
	printf("Monitor thread created with mode %s\n", (monitor_mode==0)?"AVG":"NOW");
	unsigned int nr_measur=0;
	int i=0,p_util,m_util;
	FILE * fp;
	char buffer[16];
	char buffer2[16];

	while(1) {
		if (nr_measur > 50)
			nr_measur = 0;
		i=0,p_util = m_util= 0;
		memset(buffer, 0, sizeof(buffer));
		nr_measur++;
		//Request GPU Utilization from gpu nodes
		while (gpu[i] != NULL){
			char command[255];
			sprintf(command, "ssh %s \"nvidia-smi -q -i %d -d UTILIZATION\" | grep -e \"Gpu\" -e \"Mem\" | awk '{print $3;}'", gpu[i]->node_name, gpu[i]->id);
			//printf("%s\n",command); 
			fp = popen(command,"r");
			if (fp == NULL) printf("Error popen\n");
			fgets(buffer, 8, fp);
			fgets(buffer2, 8, fp);
			p_util = atoi(buffer);
			m_util = atoi(buffer2);
			if (monitor_mode == AVG){
			//	fprintf(stdout, "Current util : %d\n",util);
				gpu[i]->proc_util  = (((gpu[i]->proc_util)*(nr_measur - 1) + p_util ) / nr_measur) ;
				gpu[i]->mem_util  = (((gpu[i]->mem_util)*(nr_measur - 1) + m_util ) / nr_measur) ;
			}
			else {
				gpu[i]->proc_util = p_util;
				gpu[i]->mem_util = m_util;
			}
			pclose(fp);
				//fprintf(stdout,"GPU @ %s : P_Util:%d M_Util:%d\n",gpu[i]->node_name, gpu[i]->proc_util, gpu[i]->mem_util);
			
			
			//printf("%d\t%s\t%d\n",n[i]->id,n[i]->name,n[i]->avail_gpus);
			i++;
		
		}
		sleep(MONITOR_REFRESH);
	}	
}
