#include "bootloader.h"

bool flip = false;

static void render(uint16_t* fb) {
  for(int i = 320*240 ; i-- ; ) {
    *(fb+i) = (flip ? 0x1FE0 : 0xF800);
  }
}

static void handleInput(uint32_t buttonStates, uint32_t buttonPresses) {
  if(buttonPresses & START) {
    flip = !flip;
    triggerRender();
  }
}

View MainMenu = {
 &render,
 &handleInput
};		       
