/********************************************
	       Header File for rgpu
	Author: Antonis Karkatsoulis
		akarkat@cslab.ece.ntua.gr
*********************************************/

#include <stdlib.h>

#define MAX_NODES 64
#define MAX_GPUS 64
#define MONITOR_REFRESH 1
#define P_FACTOR 0.8

/* ------------------------------  Monitor modes  -------------------------- *
** AVG : Keeps the average of some of the last measurements of utilizations  *
** NOW : Returns the utilization the moment it is requested		     */

enum monitor_mode
{
	AVG,
	NOW

};

/* ----------------------------   GPU struct  ------------------------------ *
** Contains info about the GPUs available in the cluster :		     *
** node_name : The name of the node the GPU is installed in.		     *
** id	     : A local id for different GPUs on the same node.		     *
** gen	     : The generation of the NVIDIA GPU. (1=G80, 2=Fermi, 3=Kepler)  *
** address   : The IP address of the node + the port that connects to this   *
**	       specific device.						     *
** proc_util : The GPU processor utilization in %.			     *
** mem_util  : The GPU memory utilization in %.				     *
** temp      : The temperature of the GPU (Celcius degrees)		     *
** ecc       : The number of ecc errors of the specific GPU		     *
** --------------------------------------------------------------------------*/

typedef struct gpu {
	char node_name[255];
	unsigned int id;
	unsigned int gen;
	char address[32];
	unsigned int proc_util;
	unsigned int mem_util;
	unsigned int temp;  //Temp and ecc measurents available only for newer Tesla Models
	unsigned int ecc;
} gpuinfo;

/* --------------------------- Node struct  -------------------------------- *
** Contains info about all the nodes present in the cluster :		     *
** id	      : A specific id for each node.				     *
** name       : The node hostname.					     *
** ip         : The IP address of the node.				     *
** avail_gpus : The available COMPATIBLE NVIDIA gpus.			     *
** screen_ids : A table with all the open screens of this node.		     *
** --------------------------------------------------------------------------*/

typedef struct node {
	unsigned int id;
	char name[255];
	char ip[32];
	int avail_gpus;
} nodeinfo;

/* Function prototypes */
void * monitor (gpuinfo **);
int usage (FILE *, char *);
void parse_nodefile (nodeinfo **, gpuinfo **, int *, int *);
int add_screen_id (nodeinfo **, int, int);
int * get_utils(gpuinfo **, int);
int export_vars (nodeinfo **, int, char *);
void inthandler (int);
int CheckForData (int);
void print_node_info (nodeinfo **, gpuinfo **, int, int);

