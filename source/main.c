#include <stdlib.h>
#include <gp2xregs.h>
#include <orcus.h>
#include <fat.h>
#include <sys/dir.h>
#include <dirent.h>
#include <unistd.h>

#include "bootloader.h"

#define MAGENTA 0xF81F
#define BLACK 0x0

// global variables
bool renderRequired = true;

// current view
View* currentView;

// settings
bool autoloadKernel;

void loadSettings() {
  // TODO - get these from the final NAND block of the bootloader
  autoloadKernel = false;
}

int main() {
  gp2xInit();
  uart_printf("\r\n\n\n**********************************\r\n* Open2x Bootloader  *\r\n**********************************\r\n\r\n");

  if(autoloadKernel && !(btnState()&START)) {
    runKernelFromNand();
    // TODO - if we reach this point, loading kernel has failed and we should show something on the screen
  }
  
  uint16_t* fb0 = malloc(320*240*2);
  uint16_t* fb1 = malloc(320*240*2);

  rgbSetPixelFormat(RGB565);
  rgbRegionNoBlend(REGION1);
  rgbSetRegionPosition(REGION1, 0, 0, 320, 240);
    for(int i = 320*240 ; i-- ; ) {
    *(fb1+i) = *(fb0+i) = 0x0;
  }
  rgbSetFbAddress((void*)fb0);
  rgbToggleRegion(REGION1, true);

  currentView = &MainMenu;

  uint16_t* currentFb = fb0;
  uint16_t* nextFb = fb1;
  uint32_t previousButtonState = 0;
  while(1) {
    uint32_t currentButtonState = btnState();
    currentView->handleInput(currentButtonState, currentButtonState&(~previousButtonState));
    previousButtonState = currentButtonState;

    if(renderRequired) {
      currentView->render(nextFb);
      lcdWaitNextVSync();
      rgbSetFbAddress((void*)nextFb);
      uint16_t* tmp = currentFb;
      currentFb = nextFb;
      nextFb = tmp;
    }
  }

  uart_printf("Testing for SD card\r\n");
  if(fatInitDefault()) {
    uart_printf("Detected %dkb SD card\r\n", sdSizeKb());
    
    DIR *dp;
    struct dirent *ep;

    dp = opendir ("sd:/");
    if (dp != NULL)
      {
	while ((ep = readdir(dp)))
	  uart_printf("%s\r\n", ep->d_name);
	(void) closedir (dp);
      }
    else
      uart_printf("Couldn't open the directory\r\n");

  } else {
    uart_printf("No SD card detected\r\n");
  }
  
  while(1) {
    uint32_t btn = btnState();
    /*    if(btn & A) uart_printf("A pressed\r\n");
    if(btn & B) uart_printf("B pressed\r\n");
    if(btn & X) uart_printf("X pressed\r\n");
    if(btn & Y) uart_printf("Y pressed\r\n");
    if(btn & START) uart_printf("START pressed\r\n");
    if(btn & SELECT) uart_printf("SELECT pressed\r\n");
    if(btn & UP) uart_printf("UP pressed\r\n");
    if(btn & DOWN) uart_printf("DOWN pressed\r\n");
    if(btn & LEFT) uart_printf("LEFT pressed\r\n");
    if(btn & RIGHT) uart_printf("RIGHT pressed\r\n");
    if(btn & UP_LEFT) uart_printf("UP_LEFT pressed\r\n");
    if(btn & UP_RIGHT) uart_printf("UP_RIGHT pressed\r\n");
    if(btn & DOWN_LEFT) uart_printf("DOWN_LEFT pressed\r\n");
    if(btn & DOWN_RIGHT) uart_printf("DOWN_RIGHT pressed\r\n");
    if(btn & VOL_UP) uart_printf("VOL_UP pressed\r\n");
    if(btn & VOL_DOWN) uart_printf("VOL_DOWN pressed\r\n");
    if(btn & STICK) uart_printf("STICK pressed\r\n");
    if(btn & L) uart_printf("L pressed\r\n");
    if(btn & R) uart_printf("R pressed\r\n");*/
    
    if(btn & START) rgbSetFbAddress(fb0);
    else if(btn & SELECT) return 0;
    else rgbSetFbAddress(fb1);
  }
}
