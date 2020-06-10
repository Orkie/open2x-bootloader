#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <fat.h>
#include <orcus.h>
#include <unistd.h>

#define BUF_SIZE 512
char inputBuffer[BUF_SIZE];
int bufferCount = 0;

extern int xmodemReceive(unsigned char *dest, int destsz);

void doHelp();
void doLs();
void doMd();
void doMw();
void doXm();

static bool startsWith(char* a, char* prefix) {
  return strncmp(prefix, a, strlen(prefix)) == 0;
}

static bool equals(char* a, char* b) {
  return strcmp(a, b) == 0;
}

void terminalNewline() {
  memset(inputBuffer, 0x00, BUF_SIZE);
  bufferCount = 0;
  printf("# ");
}

static void terminalHandleCr() {
  if(strlen(inputBuffer) == 0) {
  } else if(equals(inputBuffer, "help")) {
    doHelp();
  } else if(startsWith(inputBuffer, "ls")) {
    doLs();
  } else if(startsWith(inputBuffer, "md")) {
    doMd();
  } else if(startsWith(inputBuffer, "mw")) {
    doMw();
  } else if(equals(inputBuffer, "xm")) {
    doXm();
  } else {
    printf("Unknown command\n");
  }
  terminalNewline();
}

void terminalHandleChar(int c) {
  if(c == '\n' || c == '\r') {
    terminalHandleCr();
  } else if(c == 8 && bufferCount != 0) {
    inputBuffer[bufferCount--] = '\0';
  } else {
    if(bufferCount < BUF_SIZE) {
      inputBuffer[bufferCount++] = (char) c;
    }
  }
}

void doHelp() {
  printf("ls [path]              - Lists directory contents\n");
  printf("run [path]             - Runs a file\n");
  printf("md[whb] [addr]         - Displays the value of a memory address as a 32, 16 or 8 bit value\n");
  printf("mw[whb] [addr] [value] - Writes a value to a memory address\n");
  printf("xm                     - Receives an runs an o2x via xmodem\n");
  printf("help                   - Displays this message\n");
}

void doLs() {
  char arg[BUF_SIZE];
  int r = sscanf(inputBuffer, "ls %s", arg);
  if(r != 1) {
    printf("Invalid invocation of ls\n");
    return;
  }

  fatUnmount("sd");
  if(!fatInitDefault()) {
    printf("Couldn't open SD card\n");
  }

  DIR *d;
  struct dirent *dir;
  d = opendir(arg);
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      printf("%s\n", dir->d_name);
    }
    closedir(d);
  }
}

void doMd() {
  char* space = strchr(inputBuffer, ' ');
  if(space == NULL) {
    printf("Invalid invocation of md\n");
    return;
  }
  unsigned int address = strtoul(space, NULL, 0);  
  
  if(startsWith(inputBuffer, "mdb")) {
    printf("0x%x: 0x%x\n", address, *((uint8_t*) address));
  } else if(startsWith(inputBuffer, "mdh")) {
    printf("0x%x: 0x%x\n", address, *((uint16_t*) address));
  } else if(startsWith(inputBuffer, "mdw")) {
    printf("0x%x: 0x%lx\n", address, *((uint32_t*) address));
  } else {
    printf("Invalid width\n");
  }
}

void doMw() {
  char* space1 = strchr(inputBuffer, ' ');
  if(space1 == NULL) {
    printf("Invalid invocation of mw\n");
    return;
  }
  unsigned int address = strtoul(space1, NULL, 0);  

  char* space2 = strchr(space1+1, ' ');
  if(space2 == NULL) {
    printf("Invalid invocation of mw\n");
    return;
  }
  unsigned int value = strtoul(space2, NULL, 0);    
  
  if(startsWith(inputBuffer, "mwb")) {
    *((uint8_t*) address) = (uint8_t) value;
  } else if(startsWith(inputBuffer, "mwh")) {
    *((uint16_t*) address) = (uint16_t) value;
  } else if(startsWith(inputBuffer, "mww")) {
    *((uint32_t*) address) = (uint32_t) value;
  } else {
    printf("Invalid width\n");
  }
}

#define XM_BUF_SIZE 0x400000 // 4M
extern int launchO2xFile(FILE* fp, char* path, char* arg);
void doXm() {
  unsigned char* buffer = (unsigned char*) malloc(XM_BUF_SIZE);
  if(buffer == NULL) {
    printf("Could not allocate buffer to receive file\n");
  }
  uartSetEcho(false);

  printf("Waiting to receive file...\n");
  xmodemReceive(buffer, XM_BUF_SIZE);
  usleep(10000);
  printf("\r\nTransfer complete!\n");

  // TODO - refactor interpreter_o2x.c to allow running an in-memory binary, note that we only have 1M of space below the heap so may need to fiddle with that / make sure we don't have any sections in the heap area

  FILE* fp = fmemopen(buffer, XM_BUF_SIZE, "rb");
  int result = 0;
  if(fp != NULL) {
    result = launchO2xFile(fp, "xmodem", NULL);
    
    fclose(fp);
  } else {
    printf("Could not allocate in memory file\n");
  }

  printf("Failed to execute received O2X file: %d\n", result);
  
  uartSetEcho(true);
  free(buffer);
}
