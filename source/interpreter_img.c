#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bootloader.h"

static int launch(char* path) {
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

InternalInterpreter KernelInternalInterpreter = {
  &launch
};

Interpreter KernelInterpreter = {
 "img",
 true,
 {.internal = &KernelInternalInterpreter}
};
