#include <stdlib.h>
#include <orcus.h>
#include <gp2xregs.h>
#include "miniz.h"
#include "bootloader.h"
#include "background_bin.h"

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

int prepareImage(void* img, uint32_t* entry);
int gunzip(uint8_t* data, unsigned int length, void* dest);

#define JMP(addr) \
    __asm__("mov pc,%0" \
            : /*output*/ \
            : /*input*/ \
            "r" (addr) \
           );

void runKernelFromNand() {
  void* dest = malloc(KERNEL_REGION_BYTES);
  nandRead(0x80000, KERNEL_REGION_BYTES>>9, dest);
  uart_printf("Loaded data from NAND at 0x%x\n", dest);
  uint32_t entry;

  uint16_t* fb = (uint16_t*) 0x3100000;
  memcpy((void*) fb, (void*) background_bin, 320*240*2);
  rgbPrintf(fb, (320-(15*FONT_WIDTH))>>1, 32, 0x0000, "Starting kernel");
  rgbSetFbAddress((void*)fb);
  
  if(!prepareImage(dest, &entry)) {
    uart_printf("Executing kernel at 0x%x\n", entry);
    
    asm volatile("ldr r0, =0x0");
    asm volatile("ldr r1, =395");
    asm volatile("ldr r2, =0x0");
    JMP(entry)
  }
  
  free(dest);
  // TODO -failed
}

int prepareImage(void* img, uint32_t* entry) {
  UbootHeader* header = (UbootHeader*) img;

  void* startOfData = (void*) (header+1);

  if(B2L(header->ih_magic) != UBOOT_MAGIC_NUMBER) {
    uart_printf("Not a valid u-boot image\n");
    return 1;
  }

  if(header->ih_arch != 2) {
    uart_printf("Not an ARM image\n");
    return 2;
  }

  uart_printf("Image name is '%.32s'\n", header->ih_name);
  uart_printf("  Length: 0x%x\n", B2L(header->ih_size)); 
  uart_printf("  Load address: 0x%x\n", B2L(header->ih_load));
  uart_printf("  Entry point: 0x%x\n", B2L(header->ih_ep));
   *entry = B2L(header->ih_ep);

  if(header->ih_comp == 0) {
    uart_printf("Not compressed\n");
    // TODO - copy to destination
  } else if(header->ih_comp == 1) {
    uart_printf("GZip compressed\n");

    if(gunzip((uint8_t*) startOfData, B2L(header->ih_size), (void*)B2L(header->ih_load))) {
      uart_printf("Could not gunzip\n");
      return 4;
    }
    
    uart_printf("Successfully unzipped\n");
  } else {
    uart_printf("Unsupported compression type\n");
    return 3;
  }

  return 0;
}

int gunzip(uint8_t* data, unsigned int length, void* dest) {
  int r;
  uint8_t flg = data[3];

  if(data[0] != 0x1f || data[1] != 0x8b) {
    uart_printf("Invalid gzip data\n");
    return 1;
  }

  int baseLength = 10;
  int extraFieldLength = flg & BIT(2) ? 2 + (data[11] << 8 | data[10]) : 0;
  int nameLength = 0;
  if(flg & BIT(3)) {
    nameLength = 1; // terminating \0
    int nStart = baseLength + extraFieldLength;
    while(data[nStart++] != 0x0) {
      nameLength++;
    }
  }
  int commentLength = 0;
  if(flg & BIT(4)) {
    commentLength = 1; // terminating \0
    int cStart = baseLength + extraFieldLength + nameLength;
    while(data[cStart++] != 0x0) {
      commentLength++;
    }    
  }
  int crcLength = flg & BIT(1) ? 2 : 0;
  int headerLength = baseLength + extraFieldLength + nameLength + commentLength + crcLength;

  z_stream zs;
  zs.zalloc = Z_NULL;
  zs.zfree = Z_NULL;
  if((r = inflateInit2(&zs, -Z_DEFAULT_WINDOW_BITS)) != Z_OK) {
    uart_printf("Could not init inflate: %d\n", r);
    return 1;
  }

  zs.next_in = data + headerLength;
  zs.avail_in = length - headerLength;
  zs.next_out = (unsigned char*) dest;
  zs.avail_out = 0x400000; // TODO - tune this to our memory map

  r = inflate(&zs, Z_FINISH);
  if(r != Z_OK && r != Z_STREAM_END) {
    uart_printf("Inflate failed: %d\n", r);
    return 1;
  }

  return 0;
}
