#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bootloader.h"

#define O2X_MAGIC 0x3178326F

typedef struct {
  uint32_t length;
  uint32_t loadAddress;
} O2xSection;

typedef struct {
  uint32_t magic;
  char name[32];
  uint16_t icon[16*16];
  uint8_t reserved[8];
  uint32_t paramLength;
  uint32_t paramAddr;
  uint8_t numberOfSections;
} O2xHeader;

static int launch(char* path, char* arg) {
  if(arg != NULL) {
    uart_printf("Launching %s with arg %s\n", path, arg);
  } else {
    uart_printf("Launching %s\n", path);
  }
  
  FILE* fp = fopen(path, "rb");
  if(fp == NULL) {
    showError("Could not open file");
    return 2;
  }

  O2xHeader header;
  if(fread(&header, sizeof(O2xHeader), 1, fp) != 1) {
    showError("Could not read from file");
    fclose(fp);
    return 3;
  }

  if(header.magic != O2X_MAGIC) {
    showError("Not a valid O2X file");
    fclose(fp);
    return 4;
  }

  uint32_t* paramAddr = (uint32_t*) header.paramAddr;
  int pathLen = strlen(path);
  int requiredSpace = sizeof(uint32_t) +
    sizeof(uint32_t) * (arg == NULL ? 1 : 2) +
    sizeof(uint32_t) + // argv termination
    pathLen + 1 +
    (arg == NULL ? 0 : strlen(arg)+1);
  
  if(header.paramLength == sizeof(int)) {
     uart_printf("Passing 0 parameters", (paramAddr+1));
    *paramAddr = 0;    
  } else if(header.paramLength >= requiredSpace) {
    uart_printf("Setting argv[0] at 0x%x\n", (paramAddr+1));
    *paramAddr = arg == NULL ? 1 : 2;
    paramAddr++;
    *paramAddr = (uint32_t) paramAddr+(sizeof(uint32_t)*(arg == NULL ? 2 : 3));
    paramAddr++;
    if(arg != NULL) {
      *paramAddr = (uint32_t) paramAddr+(sizeof(uint32_t)*2) + pathLen + 1;
      paramAddr++;
    }
    *paramAddr = (uint32_t) NULL;
    paramAddr++;

    strcpy((char*)paramAddr, path);
    if(arg != NULL) {
      strcpy(((char*)paramAddr)+pathLen+1, arg);
    }
  } else {
    uart_printf("Not setting parameters\n");
  }

  O2xSection sectionHeader;
  uint32_t jumpTo = 0x0;
  for(int i  = 0 ; i < header.numberOfSections ; i++) {
    if(fread(&sectionHeader, sizeof(O2xSection), 1, fp) != 1) {
      showError("Could not read from file");
      fclose(fp);
      return 5;
    }

    uart_printf("Loading section %d at address 0x%x, %d bytes\n", i, sectionHeader.loadAddress, sectionHeader.length);

    // TODO - we need to really check if we are going to overwrite ourselves here, and load to memory then run a small piece of PIC code which copies it to its final destination

    uint32_t* dest = (uint32_t*) sectionHeader.loadAddress;
    if(fread(dest, sizeof(uint8_t), sectionHeader.length, fp) != sectionHeader.length) {
      showError("Could not read from file");
      fclose(fp);
      return 6;
    }

    // first section is special, it is the entry point for the 920t, we need to copy its vector table into place
    if(i == 0) {
      uart_printf("Copying vector table for section 0\n");
      jumpTo = sectionHeader.loadAddress;
      usleep(20000);
      uint32_t* copyTo = (uint32_t*) 0x0;
      while(*dest != 0xDEADBEEF) {
	*copyTo = *dest;
	copyTo++;
	dest++;
      }
    }
  }
  fclose(fp);

  uart_printf("Jumping to 0x%x\n", jumpTo);
  JMP(jumpTo);
  return 1;// we should never reach here
}

InternalInterpreter O2xInternalInterpreter = {
  &launch
};

Interpreter O2xInterpreter = {
 "o2x",
 true,
 {.internal = &O2xInternalInterpreter}
};
