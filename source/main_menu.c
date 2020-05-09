#include <string.h>
#include "bootloader.h"

int selected = 0;

static void browseSd() {
  transitionView(&Filer);
}

static void bootFromNand() {
  runKernelFromNand(); // TODO - we want to be able to set either boot from a gp2xkernel.img in SD root, or from NAND. If SD fails, offer NAND
}

const MenuItem menu[] = {
  {.title = "Browse SD card", .callback = &browseSd},
  {.title = "Boot Linux", .callback = &bootFromNand},
  {.title = "Settings", .callback = NULL}
};

static void render(uint16_t* fb) {
  rgbPrintf(fb, (320-(17*FONT_WIDTH))>>1, 32, 0x0000, "Open2x Bootloader");
  rgbPrintf(fb, 280-3*FONT_WIDTH, 32, 0x0000, VERSION);

  for(int i = 0 ; i < SIZEOF(menu) ; i++) {
    if(selected == i) {
      rgbPrintf(fb, 32, 56+(FONT_HEIGHT*i), 0xF800, "* ");
    }
    rgbPrintf(fb, 32+FONT_WIDTH*2, 56+(FONT_HEIGHT*i), selected == i ? 0xF800 : 0x0000, menu[i].title);
  }
}

static void handleInput(uint32_t buttonStates, uint32_t buttonPresses) {
  if(buttonPresses) {
    clearError();
  }
  
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
 &handleInput,
 NULL,
 NULL
};		       
