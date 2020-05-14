#ifndef __KERNEL_H__
#define __KERNEL_H

// will not return if successful
extern void* loadKernelFromNand();
extern int launchKernel(void* image);
extern int launchKernelFromNand();

#endif
