#include <string.h>
#include <fat.h>
#include "bootloader.h"

char currentPath[1024];

static int init(char* error) {
  if(!fatInitDefault()) {
    strcpy(error, "No SD card detected.");
    return 1;
  }

  strcpy(currentPath, "/");
  return 0;
}

static void render(uint16_t* fb) {
}

static void handleInput(uint32_t buttonStates, uint32_t buttonPresses) {
  if(buttonPresses & X && strcmp(currentPath, "/") == 0) {
    transitionView(&MainMenu);
  }
}

View Filer = {
 &render,
 &handleInput,
 &init
};		       
