#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <fat.h>

#define BUF_SIZE 512
char inputBuffer[BUF_SIZE];
int bufferCount = 0;

void doHelp();
void doLs();

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
  } else {
    printf("Unknown command\n");
  }
  terminalNewline();
}

void terminalHandleChar(int c) {
  if(c == '\n' || c == '\r') {
    terminalHandleCr();
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
  printf("kermit                 - Receives an runs an o2x via Kermit\n");
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