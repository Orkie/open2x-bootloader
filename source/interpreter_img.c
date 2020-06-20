#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bootloader.h"

static int launch(char* path, char* arg) {
  FILE* fp = fopen(path, "rb");
  if(fp == NULL) {
    showError("Could not open file");
    return 1;
  }

  fseek(fp, 0L, SEEK_END);
  int size = ftell(fp);
  rewind(fp);

  void* buf = malloc(size);
  if(buf == NULL) {
    fclose(fp);
    showError("File too large to read");
    return 2;
  }
  
  if(fread(buf, sizeof(uint8_t), size, fp) != size) {
    fclose(fp);
    free(buf);
    showError("Could not read img");
    return 3;
  }

  launchKernel(buf);
  
  fclose(fp);
  free(buf);
  showError("Not a valid kernel\n");
  return 4;
};


static int getName(char* path, char* output) {
  FILE* fp = fopen(path, "rb");
  if(fp == NULL) {
    return 1;
  }

  UbootHeader header;
  if(fread(&header, sizeof(UbootHeader), 1, fp) != 1) {
    fclose(fp);
    return 2;
  }

  if(B2L(header.ih_magic) != UBOOT_MAGIC_NUMBER) {
    fclose(fp);
    return 2;
  }

  strcpy(output, (char*)header.ih_name);
  fclose(fp);
  return 0;
}

static int getIcon(char* path, uint16_t* output) {
  FILE* fp = fopen(path, "rb");
  if(fp == NULL) {
    return 1;
  }

  UbootHeader header;
  if(fread(&header, sizeof(UbootHeader), 1, fp) != 1) {
    fclose(fp);
    return 2;
  }

  if(B2L(header.ih_magic) != UBOOT_MAGIC_NUMBER) {
    fclose(fp);
    return 2;
  }

  fclose(fp);
  memcpy(output, linux_bin, 16*16*2);
  return 0;
}

InternalInterpreter KernelInternalInterpreter = {
						 &launch,
						 &getName,
						 &getIcon
};

Interpreter KernelInterpreter = {
 "img",
 true,
 {.internal = &KernelInternalInterpreter}
};
