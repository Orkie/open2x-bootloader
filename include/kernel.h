#ifndef __KERNEL_H__
#define __KERNEL_H

#define KERNEL_REGION_BYTES 0x120000
#define UBOOT_MAGIC_NUMBER 0x27051956
#define GP2X_KERNEL_ARCH 395

#define B2L(n) (((n & 0xff000000) >> 24) | ((n & 0x00ff0000) >> 8) | ((n & 0x0000ff00) << 8) | (n << 24))

typedef struct {
  uint32_t ih_magic; /* Image Header Magic Number */
  uint32_t ih_hcrc; /* Image Header CRC Checksum */
  uint32_t ih_time; /* Image Creation Timestamp */
  uint32_t ih_size; /* Image Data Size */
  uint32_t ih_load; /* Data Load  Address */
  uint32_t ih_ep; /* Entry Point Address */
  uint32_t ih_dcrc; /* Image Data CRC Checksum */
  uint8_t ih_os; /* Operating System */
  uint8_t ih_arch; /* CPU architecture */
  uint8_t ih_type; /* Image Type */
  uint8_t ih_comp; /* Compression Type */
  uint8_t ih_name[32]; /* Image Name */
} UbootHeader;

// will not return if successful
extern void* loadKernelFromNand();
extern int launchKernel(void* image);
extern int launchKernelFromNand();

#endif
