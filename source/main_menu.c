#include <string.h>
#include "bootloader.h"

int selected = 0;

void doNothing() {
  uart_printf("selected this\n");
}

const MenuItem menu[] = {
  {.title = "Browse SD card", .callback = NULL},
  {.title = "Boot from NAND", .callback = &doNothing},
  {.title = "Settings", .callback = NULL}
};

static void render(uint16_t* fb) {
  memcpy((void*) fb, (void*) background_bin, 320*240*2);
  displayPrintf(fb, (320-(17*CHAR_WIDTH))>>1, 32, 0x0000, "Open2x Bootloader", 2020);
  displayPrintf(fb, 280-3*CHAR_WIDTH, 32, 0x0000, VERSION);

  for(int i = 0 ; i < SIZEOF(menu) ; i++) {
    if(selected == i) {
      displayPrintf(fb, 32, 56+(CHAR_HEIGHT*i), 0xF800, "* ");
    }
    displayPrintf(fb, 32+CHAR_WIDTH*2, 56+(CHAR_HEIGHT*i), selected == i ? 0xF800 : 0x0000, menu[i].title);
  }
}

static void handleInput(uint32_t buttonStates, uint32_t buttonPresses) {
  if(buttonPresses & UP) {
    selected = selected == 0 ? 0 : selected - 1;
    triggerRender();
  }

  if(buttonPresses & DOWN) {
    selected = selected == SIZEOF(menu)-1 ? selected : selected + 1;
    triggerRender();
  }

  if(buttonPresses & B || buttonPresses & START) {
    if(menu[selected].callback != NULL) {
      menu[selected].callback();
    }
  }
}

View MainMenu = {
 &render,
 &handleInput
};		       
