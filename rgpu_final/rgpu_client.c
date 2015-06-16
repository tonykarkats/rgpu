/********************************************
	 rgpu Client v 1.0
	Author: Antonis Karkatsoulis
		akarkat@cslab.ece.ntua.gr
*********************************************/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "rgpu.h"
 
#include <getopt.h>


int usage(FILE *std,char *appname){
 
	char *msg="Usage: %s [OPTION]...  \n\
	  -V version (--version)\n\
	  -h help (--help)\n\
	  -p port (--port)\n\
	  -v verbose (--verbose)\n\
	  -r executable (--run)\n";
 
	fprintf(std,msg,appname);
 
	return 1;
}
 
int main(int argc, char *argv[]){
	int sockfd=0;
	char data[512]={0};
	char executable[255];
	struct hostent *he;
	struct sockaddr_in their_addr;
	char hostname[64];
	char *filename = NULL;
	char *appname=argv[0];
	int port=1091;
	char *screen_id =NULL;
	char command[255]; 
	int verbose=0;
	

 
	/* Flag set by `--verbose'. */
	static int verbose_flag;
 
	static int c;
 
	int status=0;	
 
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
			{"port",     required_argument, 0, 'p'},
			{"executable",required_argument, 0, 's'},
			{0, 0, 0, 0}
		};
 
		/* getopt_long stores the option index here. */
		int option_index = 0;
 
		c = getopt_long(argc, argv, "hV:r:p:v",long_options, &option_index);
 
		/* Detect the end of the options. */
		if (c == -1)
			break;
 
		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
 
				printf ("option %s", long_options[option_index].name);
 
				if (optarg)
					printf (" with arg %s", optarg);
 
				printf ("\n");
				break;
 
			case 'h':
				usage(stdout,appname);
				return -1;/*OTHERS*/
				break;
 
			case 'V':
				
				printf(" ----------------------------------------------\n");
				printf("|	     rgpu Client v 1.0	 	       |\n");
				printf("| Antonis Karkatsoulis (akarkat@cslab.ntua.gr) | \n");
				printf(" ----------------------------------------------\n");
				return EXIT_SUCCESS;
				break;			
 
			case 'p':
				port=atoi(optarg);
				break;
 
			case 'r':
				strcpy(executable, optarg);
				printf("Executable %s\n", executable);
				break; 
			case 'v':
				verbose++;
				break;
 
			default:
				abort();
		}
	}
 

	strcpy(hostname, getenv("RGPU_HYPERV"));	

	 
	if(hostname==NULL){
		usage(stderr,appname);
		return -1;/*OTHERS*/
	}
 
	if((he= gethostbyname(hostname)) == NULL){
		printf("Error with gethostbyname()\n");
		return -1;/*OTHERS*/
	}
 

	if((sockfd = socket(AF_INET,SOCK_STREAM, 0)) == -1){
		printf("Error with socket()\n");
		return -1;/*UNKNOWN*/
	}
 
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(their_addr.sin_zero,0,sizeof their_addr.sin_zero);
 
	if(connect(sockfd,(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1){
		/*printf("Error with connect()\n");*/
		printf("Not able to establish a communication with the Hypervisor!\n");
		return -1;/*UNKNOWN*/
	}
 
	//Sending part
	printf("Communication with Hypervisor established. Waiting to receive..\n");


	memset(data,0,sizeof data);
 
	if(recv(sockfd,data,sizeof data, 0) == -1){
		printf("Error with recv()\n");
		return -1;/*UNKNOWN*/
	}


//	printf("Received: %s with size %d\n",data, sizeof(data));
	printf("Running rCUDA..\n");	
	char temp[512];
	char *token = strtok(data,"-");
	sprintf(command, "export RCUDA_DEVICE_COUNT=%s ;", token);
	token = strtok(NULL, "-");
	int d=0;
	while (token) {
	//	printf("IP: %s\n", token);
		sprintf(temp, "export RCUDA_DEVICE_%d=%s ;",d, token);
		strcat(command, temp);
		token = strtok(NULL, "-");	 
		d++;
	//	sprintf(command, "export RCUDA_DEVICE_COUNT=1 ; export RCUDA_DEVICE_0=%s ; %s", data, executable);
	}
	strcat(command, executable);

//	printf("%s", command);
	if (system(command) == -1)
		printf("Error exporting rCUDA environment variables\n");
 
  
	shutdown(sockfd,2);
	return status;
}


