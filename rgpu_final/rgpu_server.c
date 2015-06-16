/********************************************
	 rgpu Server v 1.0
	Author: Antonis Karkatsoulis
		akarkat@cslab.ece.ntua.gr
*********************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
/*Libraries for communications*/
#include <signal.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
/*Libraries for the daemon*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
 
/*Library for syslog*/
#include <syslog.h>
 
/*library for options*/
#include <getopt.h>

/*library for file handles*/
#include <libgen.h>

/*rgpu Library */
#include "rgpu.h"
 

volatile sig_atomic_t done;
unsigned int monitor_mode = NOW;
int added;

/* Prints usage information about the rgpu_server executable */
 
int usage(FILE *std,char *appname){
 
	char *msg="Usage: %s [OPTION] \n\
	  -D run as daemon (--daemon)\n\
	  -s log handled by syslog (--syslog)\n\
	  -V version (--version)\n\
	  -p port (--port)\n\
	  -m monitor mode(AVG or NOW)\n\
	  -h help (--help)\n\
	  -v verbose (--verbose)\n";
 
	fprintf(std,msg,appname);
 
	return 0;
}

/* Parses the nodefile and creates the necessary structs				 *
** These are :										 *
** 1) nodeinfo structs, containing all the information about the nodes of the cluster    *
** 2) gpuinfo structs, containing information about the GPUs available		 	 */

void parse_nodefile(nodeinfo *n[], gpuinfo *gpu[], int *num_nodes, int *num_gpus) {	 

	int i=0, j=0, k=0, av_gpus=0;
	size_t len = 0;
	char * line = NULL;
	char name[255];
	char ip[32];
	FILE * fd = NULL;
	*num_gpus = 0;

	//Read configuration file
	if ((fd = fopen("nodes.config","r")) == NULL) {
		perror("Error opening node configuration file");
		exit(EXIT_FAILURE);
	}
	while (getline(&line,&len,fd) != -1) {	
		if (line[0] == '#') continue;
		n[i] = (nodeinfo *)malloc(sizeof(nodeinfo));
		sscanf(line,"%d %s %s %d",&n[i]->id, name, ip, &av_gpus);
		//printf("Available GPUs: %d\n", av_gpus);
		n[i]->avail_gpus = av_gpus;
		strcpy(n[i]->name, name);
		strcpy(n[i]->ip, ip);
		// Each GPU in the same node is assigned a different ID, so that
		// nodes with multiple GPUs can be monitored.
		for (j=0; j< av_gpus; j++){
			gpu[k] = (gpuinfo*)malloc(sizeof (gpuinfo));
			gpu[k]->id = j;
			gpu[k]->proc_util=0;
			gpu[k]->mem_util=0;
			gpu[k]->gen=0;
			gpu[k]->ecc=0;
			gpu[k]->temp=0;
			strcpy(gpu[k]->node_name, name);
			//to fix if ports will be used in the future
			strcpy(gpu[k]->address, ip);
			if (j != 0){
				char str[2];
				sprintf(str, "%d", j);
				strcat(gpu[k]->address, str);
			}
			*num_gpus += 1;
			k++;
			//printf("Added %s with address %s\n", gpu[j]->node_name, gpu[j]->address);
		}
		
		i++;

	}

	*num_nodes = i;
}

/* Gathers the utilizaton percentages of the GPUs from the monitor system  *
** The selection is based upon the utilization percentages of the GPUs     *
** and their free memory. Tuning the P_FACTOR variable, one can choose     *
** which of these is most important for the final decision. A table        *
** containing the ids of the GPUs in ascending utilization	           */

int * get_utils(gpuinfo *gpu[], int num_gpus) {

	int i,j,tempo;
	float temp;
	float utils[num_gpus];
	int gpu_utils[num_gpus];

	for (i=0; i<num_gpus; i++) {
		gpu_utils[i] = i;
		utils[i] = gpu[i]->proc_util*P_FACTOR + gpu[i]->mem_util*(1-P_FACTOR);	
	}

	/* Bubblesort that returns the indices sorted */

	for (i=0; i<num_gpus; i++) {
		for (j=0; j<num_gpus-1; j++) {
			if (utils[j] > utils[j+1]){
				temp = utils[j+1];
				utils[j+1] = utils[j];
				utils[j] = temp;
				tempo = gpu_utils[j+1];
				gpu_utils[j+1] = gpu_utils[j];
				gpu_utils[j] = tempo;
			}

		}


	}

//	for (i=0;i<num_gpus;i++)
//		printf("%d\n", gpu_utils[i]);

	return &gpu_utils;
}


void inthandler(int Sig){
	done=1;
}

/* Checks the socket for new data concerning a new client connection */
 
int CheckForData(int sockfd){

	struct timeval tv;
	fd_set read_fd;
	tv.tv_sec=0;
	tv.tv_usec=0;
	FD_ZERO(&read_fd);
	FD_SET(sockfd, &read_fd);
	if(select(sockfd+1, &read_fd, NULL, NULL, &tv) ==  -1){
		return 0;
	}
	if(FD_ISSET(sockfd,&read_fd)){
		return 1;
	}
	return 0;
}

/* Prints all the information about the nodes of the cluster as well *
** as the state of the GPUs available.				     */

void print_node_info(nodeinfo **nodes, gpuinfo **gpus, int num_nodes, int num_gpus){
	printf("     List of available nodes    \n");
	printf("--------------------------------\n");
	printf("ID | Name          | IP            | GPUs |\n");
	printf("------------------------------------------|\n");
	int i;
	for (i=0;i<num_nodes;i++)
		printf("%-3d| %-14s| %-14s| %-5d|\n",nodes[i]->id,nodes[i]->name,nodes[i]->ip,nodes[i]->avail_gpus);
	printf("-------------------------------------------\n");
	//printf("Num GPUs = %d\n", num_gpus);
	printf("\nList of available GPUs\n");
	printf("----------------------\n");
	printf("ID | Hostname  \n");
	printf("---------------\n");
	for (i=0; i<num_gpus; i++)
		printf("%-3d| %s\n", i, gpus[i]->node_name);
}

 
int main(int argc, char **argv){

	char *appname="rgpud";
	static int verbose_flag=0; /* Flag set by `--verbose'. */
	static int syslog_flag=0; /* Flag set by --syslog */
	int demonize=0;
	int port=1091;
	char data[512]; /*Miscellaneous use*/
	int num_nodes, num_gpus; /* Number of nodes and GPUs */

	FILE *f_pid;
	char f_pid_name[FILENAME_MAX]="/var/run/";
	strcat(f_pid_name,appname);
	strcat(f_pid_name,".pid");


	static int c;
 
	while (1) {
		static struct option long_options[] =
		{
			/* These options set a flag. */
			{"verbose", no_argument,       &verbose_flag, 1},
			{"brief",   no_argument,       &verbose_flag, 0},

			/* These options don't set a flag.
			 * We distinguish them by their indices. */
			{"help",     no_argument,       0, 'h'},
			{"version",  no_argument,       0, 'V'},
			{"daemon",   no_argument, 0, 'D'},
			{"syslog",   no_argument, 0, 's'},
			{"monitor", required_argument, 0, 'm'},
			{"port",  required_argument, 0, 'p'},
			{0, 0, 0, 0}
		};
 
		/* getopt_long stores the option index here. */
		int option_index = 0;
 
		c = getopt_long(argc, argv, "hVDvam:sp:",long_options, &option_index);
 
		/* Detect the end of the options. */
		if (c == -1)
			break;
 
		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
 
				printf("option %s", long_options[option_index].name);
 
				if (optarg)
					printf(" with arg %s", optarg);
 
				printf("\n");
				break;
 
			case 'h':
				usage(stdout,appname);
				return EXIT_SUCCESS;
				break;

			case 'V':/*Version*/
				printf(" ----------------------------------------------\n");
				printf("|	     rgpu Server v 1.0	 	       |\n");
				printf("| Antonis Karkatsoulis (akarkat@cslab.ntua.gr) | \n");
				printf(" ----------------------------------------------\n");
				return EXIT_SUCCESS;
				break;
 
			case 'p':/*Port*/
				port=atoi(optarg);
				break;
 
			case 'D':
				demonize=1;
				syslog_flag=1;
				break;

			case 'm':/*Monitor Mode*/
				if (strcmp(optarg, "AVG") == 0)
					monitor_mode = AVG;
				else if (strcmp(optarg, "NOW") == 0)
					monitor_mode = NOW;
				else {
					usage(stdout,appname);
					abort();
				}
				break;
 			
			case 'v':
				verbose_flag++;
				break;
 
			case 's':
				syslog_flag=1;
				break;
 
			default:
				abort();
		}
	}
 
	/* Open any logs here */
	if (syslog_flag == 1)
		syslog(LOG_NOTICE, " started by User %d", getuid());
	else
		printf("%s started by User %d\n",appname,getuid());
 
	/* verbosity?? */
	if(verbose_flag){
		if (syslog_flag == 1)
			syslog(LOG_NOTICE, " verbosity set to %i", verbose_flag);
		else
			printf("Verbosity set to %i\n",verbose_flag);
	}
 
	if(demonize){
		/*ONLY SYSLOG EXISTS HERE*/
 
		/* Our process ID and Session ID */
		pid_t pid, sid;
 
		/* Fork off the parent process */
		pid = fork();
		if (pid < 0) {
			exit(EXIT_FAILURE);
		}
 
		/* If we got a good PID, then
		 * we can exit the parent process. */
		if (pid > 0) {
			/*Write PID in /var/run/xxx.pid*/
			f_pid=fopen(f_pid_name,"w");
			if(f_pid != NULL){
				fprintf(f_pid,"%i\n",pid);
				fclose(f_pid);
			}else{
				syslog(LOG_NOTICE, " opening %s file failed!", f_pid_name);
			}
			exit(EXIT_SUCCESS);
		}
 
		/* Change the file mode mask */
		umask(0);
		
		/* Change the daemon's name */
		strcpy(argv[0], "rgpud");
 
		/* Create a new SID for the child process */
		sid = setsid();
		if (sid < 0) {
			syslog(LOG_NOTICE, " failed to create new session!");
			exit(EXIT_FAILURE);
		}
 
		/* Change the current working directory */
		if ((chdir("/")) < 0) {
			syslog(LOG_NOTICE, " failed to change curr directory to /!");
			exit(EXIT_FAILURE);
		}
 
		/* Close out the standard file descriptors */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}
 
	/* Daemon-specific initialization goes here */
 
	/* Socket Initializations */
	int sockfd=0;
	int new_fd=0;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	socklen_t sin_size=0;
 
	if(signal(SIGINT,SIG_IGN) != SIG_IGN){
		signal(SIGINT,inthandler);
	}
 
	if(signal(SIGTERM,SIG_IGN) != SIG_IGN){
		signal(SIGTERM,inthandler);
	}

	
	/*  Open nodefile and create the necessary structs */
	nodeinfo *nodes[MAX_NODES];
	gpuinfo *gpus[MAX_GPUS];
	parse_nodefile(nodes, gpus, &num_nodes, &num_gpus);
	int *utils = (int*)malloc(num_gpus*sizeof(int)) ; 
	print_node_info(nodes, gpus, num_nodes, num_gpus);

		/* Initialize rCUDA daemon in every GPU server */
		char * rcuda_path ;
		char command[255];
		int x;
		rcuda_path = getenv("RCUDA_PATH");
		if (rcuda_path == NULL) {
			printf("Please set RCUDA_PATH environment variable!\n");
			exit(-1);
		}
 
		printf("\nInitializing rCUDA on all GPU servers..\n");
		printf("---------------------------------------\n");

		for (x=0; x<num_nodes ; x++)
			if (nodes[x]->avail_gpus>0) {
				sprintf(command, "ssh %s 'cd %s ; ./rCUDAd'", nodes[x]->name, rcuda_path, rcuda_path);
			//	printf("%s\n", command);
				system(command);
	
		} 

		printf("---------------------------------------\n");    

	/* Monitor thread creation. This thread runs separately as it has to  *
	** constantly monitor the resources and update the GPU state structs  */

	pthread_t monitor_thread;
	int mt;
	mt = pthread_create(&monitor_thread, NULL, monitor, (void**) gpus);

	/* Note that threads never have to join because the are exiting upon completion of their job */
 
	/* Create and bind the server socket that listens for client connections */

	my_addr.sin_family=AF_INET;
	my_addr.sin_addr.s_addr=INADDR_ANY;
	my_addr.sin_port=htons(port);
 
	memset(my_addr.sin_zero,0,sizeof my_addr.sin_zero);
 
	if((sockfd=socket(AF_INET,SOCK_STREAM,0)) == -1){
		if (syslog_flag == 1)
			syslog(LOG_NOTICE, " Unexpected error on socket()");
		else
			printf("Unexpected error on socket()\n");
 
		return EXIT_FAILURE;
	}
 
	if(bind(sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) == -1){
		if (syslog_flag == 1)
			syslog(LOG_NOTICE, " Unexpected error on bind(), errno = %d",errno);
		else
			printf("Unexpected error on bind(), errno = %d\n",errno);
 
		return EXIT_FAILURE;
	}
 
	if(listen(sockfd,4)==-1){
		if (syslog_flag == 1)
			syslog(LOG_NOTICE, " Unexpected error on listen()");
		else
			printf("Unexpected error on listen()\n");
 
		shutdown(sockfd,2);
		return EXIT_FAILURE;
	}
 
	/* The Big Loop */
	while(!done){
 
		/*ICI*/
 
		if(0!=CheckForData(sockfd)){
			sin_size=sizeof(struct sockaddr_in);
			if((new_fd=accept(sockfd,(struct sockaddr *)&their_addr,&sin_size)) == -1){
				if (syslog_flag == 1)
					syslog(LOG_NOTICE, " Unexpected error on accept()");
				else
					printf("Unexpected error on accept()\n");
				continue;
			}

			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(their_addr.sin_addr), str, INET_ADDRSTRLEN);
	
			printf("Received request from node with IP: %s\nSending Util Table...\n", str);			

			utils = get_utils(gpus, num_gpus);
			int h,l;
			sprintf(data, "%d-", num_gpus);
			for (h=0; h<num_gpus; h++){
				l = utils[h];
				strcat(data, gpus[l]->address);
				strcat(data,"-");
				printf("%d %s\n", l, gpus[l]->address);
				
			}
			

			if(send(new_fd,data,sizeof data, 0) == -1){
				if (syslog_flag == 1)
					syslog(LOG_NOTICE, " Unexpected error on send()");
				else
					printf("Unexpected error on send()\n");
			}
			printf("Done!\n");
 
			shutdown(new_fd,2);
		}


		sleep(1); /* Interval between succesive calls */
	}
 
	/* Shutdown the sockets, free the memory and kill the process */
	shutdown(sockfd,2);
	remove(f_pid_name);
 
	if (syslog_flag == 1)
		syslog(LOG_NOTICE, " User requested program to halt.");
	else
		printf("User requested program to halt.\n");
 
	exit(EXIT_SUCCESS);
}

 
