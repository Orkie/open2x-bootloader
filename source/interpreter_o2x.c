#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bootloader.h"

// Data cache is 16K
#define BUFFER_SIZE 15*1024

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

int launchO2xFile(FILE* fp, char* path, char* arg) {
  O2xHeader header;
  if(fread(&header, sizeof(O2xHeader), 1, fp) != 1) {
    showError("Could not read from file");
    return 3;
  }

  if(header.magic != O2X_MAGIC) {
    showError("Not a valid O2X file");
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
     uartPrintf("Passing 0 parameters", (paramAddr+1));
    *paramAddr = 0;    
  } else if(header.paramLength >= requiredSpace) {
    uartPrintf("Setting argv[0] at 0x%x\n", (paramAddr+1));
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
    uartPrintf("Not setting parameters\n");
  }

  O2xSection sectionHeader;
  uint32_t jumpTo = 0x0;
  for(int i  = 0 ; i < header.numberOfSections ; i++) {
    if(fread(&sectionHeader, sizeof(O2xSection), 1, fp) != 1) {
      showError("Could not read from file");
      return 5;
    }

    uartPrintf("Loading section %d at address 0x%x, %d bytes\n", i, sectionHeader.loadAddress, sectionHeader.length);

    // TODO - we need to really check if we are going to overwrite ourselves here, and load to memory then run a small piece of PIC code which copies it to its final destination

    uint32_t* dest = (uint32_t*) sectionHeader.loadAddress;
    uint8_t buf[BUFFER_SIZE];
    uint32_t remainingBytes = sectionHeader.length;
    uint8_t* writeTo = (uint8_t*) sectionHeader.loadAddress;
    while(remainingBytes != 0) {
      unsigned int toRead = remainingBytes > BUFFER_SIZE ? BUFFER_SIZE : remainingBytes;
      if(fread(buf, sizeof(uint8_t), toRead, fp) != toRead) {
	showError("Could not read from file");
	return 6;
      }

      memcpy(writeTo, buf, (remainingBytes > BUFFER_SIZE) ? BUFFER_SIZE : remainingBytes);
      writeTo += BUFFER_SIZE;
      remainingBytes = remainingBytes > BUFFER_SIZE ? remainingBytes - BUFFER_SIZE : 0;
    }

    // first section is special, it is the entry point for the 920t, we need to copy its vector table into place
    if(i == 0) {
      uartPrintf("Copying vector table for section 0\n");
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

  uartPrintf("Jumping to 0x%x\n", jumpTo);
  cacheCleanD();
  mmuDisable();
  cacheDisableI();
  cacheInvalidateDI();
  JMP(jumpTo);
  return 1;// we should never reach here
}

static int launch(char* path, char* arg) {
  if(arg != NULL) {
    uartPrintf("Launching %s with arg %s\n", path, arg);
  } else {
    uartPrintf("Launching %s\n", path);
  }
  
  FILE* fp = fopen(path, "rb");
  if(fp == NULL) {
    showError("Could not open file");
    return 2;
  }

  int r = launchO2xFile(fp, path, arg);
  fclose(fp);
  return r;
}

static int getName(char* path, char* output) {
  FILE* fp = fopen(path, "rb");
  if(fp == NULL) {
    return 1;
  }

  O2xHeader header;
  if(fread(&header, sizeof(O2xHeader), 1, fp) != 1) {
    fclose(fp);
    return 2;
  }

  if(header.magic != O2X_MAGIC) {
    fclose(fp);
    return 3;
  }

  strcpy(output, header.name);
  fclose(fp);
  return 0;
}

static int getIcon(char* path, uint16_t* output) {
  FILE* fp = fopen(path, "rb");
  if(fp == NULL) {
    return 1;
  }

  O2xHeader header;
  if(fread(&header, sizeof(O2xHeader), 1, fp) != 1) {
    fclose(fp);
    return 2;
  }

  if(header.magic != O2X_MAGIC) {
    fclose(fp);
    return 3;
  }

  memcpy(output, header.icon, 16*16*2);
  fclose(fp);
  return 0;
}

InternalInterpreter O2xInternalInterpreter = {
					      &launch,
					      &getName,
					      &getIcon
};

Interpreter O2xInterpreter = {
 "o2x",
 true,
 {.internal = &O2xInternalInterpreter}
};
