#include <cuda_runtime.h>
#include <stdio.h>
#include <cuda.h>

cudaError_t cudaGetDeviceProperties(struct cudaDeviceProp *prop, int device) {
	printf("Hooked\n");
	return 0;
//	return cudaGetDeviceProperties(prop, device);

}
