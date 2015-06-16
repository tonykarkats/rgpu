#include <stdio.h>
#include <dlfcn.h>

#include "elf_hook.h"

#define LIBCUDA_PATH "libcudart.so"

void rCUDAInit();

int hooked_CUDAInit()
{
    CUDAInit(); 

    puts("is HOOKED!");
}

int main()
{
    void *handle = dlopen(LIBCUDA_PATH, RTLD_LAZY);
    void *original1;

    if (NULL == handle)
        fprintf(stderr, "Failed to open \"%s\!\n", LIBCUDA_PATH);


    original = elf_hook(LIBCUDA_PATH, LIBRARY_ADDRESS_BY_HANDLE(handle), "CUDAInit", hooked_CUDAInit);

    if (NULL == original)
        fprintf(stderr, "Redirection failed!\n");


    original = elf_hook(LIBCUDA_PATH, LIBRARY_ADDRESS_BY_HANDLE(handle), "CUDAInit", hooked_CUDAInit);

    if (NULL == original)
    //both pointers should contain hooked_puts() address now
        fprintf(stderr, "Restoration failed!\n");

    CUDAInit();  //again calls puts() from libc.so twice

    dlclose(handle);

    return 0;
}
