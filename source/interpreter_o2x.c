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

static int getName(char* path, char* dest) {
  FILE* fp = fopen(path, "rb");
  if(fp == NULL) {
    showError("Could not open file");
    return 1;
  }

  O2xHeader header;
  if(fread(&header, sizeof(O2xHeader), 1, fp) != 1) {
    showError("Could not read from file");
    fclose(fp);
    return 2;
  }

  if(header.magic != O2X_MAGIC) {
    showError("Not a valid O2X file");
    fclose(fp);
    return 3;
  }
  
  strncpy(dest, header.name, 32);
  
  fclose(fp);
  return 0;
}

static int launch(char* path) {
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

  O2xSection sectionHeader;
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
  
  JMP(0x0);
  return 1;// we should never reach here
}

InternalInterpreter O2xInternalInterpreter = {
  &launch,
  &getName
};

Interpreter O2xInterpreter = {
 "o2x",
 true,
 {.internal = &O2xInternalInterpreter}
};
