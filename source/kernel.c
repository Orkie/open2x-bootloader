#include <stdlib.h>
#include <orcus.h>
#include <gp2xregs.h>
#include "miniz.h"
#include "bootloader.h"
#include "background_bin.h"

int prepareImage(void* img, uint32_t* entry);
int gunzip(uint8_t* data, unsigned int length, void* dest);

int launchKernel(void* image) {
  uint32_t entry;

  uint16_t* fb = (uint16_t*) 0x3100000;
  memcpy((void*) fb, (void*) background_bin, 320*240*2);
  rgbPrintf(fb, (320-(15*FONT_WIDTH))>>1, 32, 0x0000, "Starting kernel");
  rgbSetFbAddress((void*)fb);

  int result = prepareImage(image, &entry);
  if(!result) {
    uartPrintf("Executing kernel at 0x%x\n", entry);
    
    asm volatile("ldr r0, =0x0");
    asm volatile("ldr r1, =395");
    asm volatile("ldr r2, =0x0");
    JMP(entry)
  }
  return result;
}

void* loadKernelFromNand() {
  void* dest = malloc(KERNEL_REGION_BYTES);
  nandRead(0x80000, KERNEL_REGION_BYTES>>9, dest);
  uartPrintf("Loaded data from NAND at 0x%x\n", dest);
  return dest;
}

int launchKernelFromNand() {
  void* image = loadKernelFromNand();
  int result = launchKernel(image);
  free(image);
  return result;
}

int prepareImage(void* img, uint32_t* entry) {
  UbootHeader* header = (UbootHeader*) img;

  void* startOfData = (void*) (header+1);

  if(B2L(header->ih_magic) != UBOOT_MAGIC_NUMBER) {
    uartPrintf("Not a valid u-boot image\n");
    return 1;
  }

  if(header->ih_arch != 2) {
    uartPrintf("Not an ARM image\n");
    return 2;
  }

  uartPrintf("Image name is '%.32s'\n", header->ih_name);
  uartPrintf("  Length: 0x%x\n", B2L(header->ih_size)); 
  uartPrintf("  Load address: 0x%x\n", B2L(header->ih_load));
  uartPrintf("  Entry point: 0x%x\n", B2L(header->ih_ep));
   *entry = B2L(header->ih_ep);

  if(header->ih_comp == 0) {
    uartPrintf("Not compressed\n");
    memcpy((void*)B2L(header->ih_load), startOfData, B2L(header->ih_size));
  } else if(header->ih_comp == 1) {
    uartPrintf("GZip compressed\n");

    if(gunzip((uint8_t*) startOfData, B2L(header->ih_size), (void*)B2L(header->ih_load))) {
      uartPrintf("Could not gunzip\n");
      return 4;
    }
    
    uartPrintf("Successfully unzipped\n");
  } else {
    uartPrintf("Unsupported compression type\n");
    return 3;
  }

  return 0;
}

int gunzip(uint8_t* data, unsigned int length, void* dest) {
  int r;
  uint8_t flg = data[3];

  if(data[0] != 0x1f || data[1] != 0x8b) {
    uartPrintf("Invalid gzip data\n");
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
    uartPrintf("Could not init inflate: %d\n", r);
    return 1;
  }

  zs.next_in = data + headerLength;
  zs.avail_in = length - headerLength;
  zs.next_out = (unsigned char*) dest;
  zs.avail_out = 0x400000; // TODO - tune this to our memory map

  r = inflate(&zs, Z_FINISH);
  if(r != Z_OK && r != Z_STREAM_END) {
    uartPrintf("Inflate failed: %d\n", r);
    return 1;
  }

  return 0;
}
