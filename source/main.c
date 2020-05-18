#include <stdlib.h>
#include <gp2xregs.h>
#include <orcus.h>
#include <fat.h>
#include <sys/dir.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "font_bin.h"
#include "bootloader.h"

// global variables
bool renderRequired = true;

// current view
View* currentView = NULL;
char errorMessage[256];
bool displayError = false;

// settings
bool autoloadKernel;

void loadSettings() {
  // TODO - get these from the final NAND block of the bootloader
  autoloadKernel = false;
}

int main() {
  gp2xInit();
  uart_printf("\n\n**********************************\nOpen2x Bootloader %s\n**********************************\n", VERSION);
  uart_printf("Auto load kernel: %s\n", autoloadKernel ? "on" : "off");
  uart_printf("**********************************\n");

  if(autoloadKernel && !(btnState()&START)) {
    launchKernelFromNand();
    uart_printf("Failed to load kernel from NAND!\n");
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
  rgbSetFont((uint16_t*)font_bin, FONT_WIDTH, FONT_HEIGHT);

  transitionView(&MainMenu);

  uint16_t* currentFb = fb0;
  uint16_t* nextFb = fb1;
  uint32_t previousButtonState = 0;
  while(1) {
    uint32_t currentButtonState = btnState();
    uint32_t nextButtonState;
    while(1) {
      usleep(5000);
      nextButtonState = btnState();
      if(nextButtonState == currentButtonState) {
	break;
      }
      currentButtonState = nextButtonState;
    }
    
    if(currentView != NULL) {
      currentView->handleInput(currentButtonState, currentButtonState&(~previousButtonState));
    }
    previousButtonState = currentButtonState;

    if(renderRequired) {
      memcpy((void*) nextFb, (void*) background_bin, 320*240*2);
      if(currentView != NULL) {
	currentView->render(nextFb);
      }

      if(displayError) {
	int errorChars = strlen(errorMessage);
	int errorLines = 0;
	int longestLine = 0;
	
	int lineCharCount = 0;
	for(int i = 0 ; i < errorChars ; i++) {
	  if(errorMessage[i] == '\n') {
	    errorLines++;
	    if(lineCharCount > longestLine) {
	      longestLine = lineCharCount;
	    }
	    lineCharCount = 0;
	  }
	  lineCharCount++;
	}
	
	rgbPrintf(nextFb,
		  21+((218-21)/2 - (FONT_WIDTH*longestLine)/2),
		  174+((218-174)/2 - (FONT_HEIGHT*errorLines)/2),
		  RED,
		  errorMessage);
      }
      
      lcdWaitNextVSync();
      rgbSetFbAddress((void*)nextFb);
      uint16_t* tmp = currentFb;
      currentFb = nextFb;
      nextFb = tmp;

      renderRequired = false;
    }
  }

  uart_printf("Testing for SD card\n");
  if(fatInitDefault()) {
    uart_printf("Detected %dkb SD card\n", sdSizeKb());
    
    DIR *dp;
    struct dirent *ep;

    dp = opendir ("sd:/");
    if (dp != NULL)
      {
	while ((ep = readdir(dp)))
	  uart_printf("%s\n", ep->d_name);
	(void) closedir (dp);
      }
    else
      uart_printf("Couldn't open the directory\n");

  } else {
    uart_printf("No SD card detected\n");
  }
  
  while(1) {
    uint32_t btn = btnState();
    /*    if(btn & A) uart_printf("A pressed\n");
    if(btn & B) uart_printf("B pressed\n");
    if(btn & X) uart_printf("X pressed\n");
    if(btn & Y) uart_printf("Y pressed\n");
    if(btn & START) uart_printf("START pressed\n");
    if(btn & SELECT) uart_printf("SELECT pressed\n");
    if(btn & UP) uart_printf("UP pressed\n");
    if(btn & DOWN) uart_printf("DOWN pressed\n");
    if(btn & LEFT) uart_printf("LEFT pressed\n");
    if(btn & RIGHT) uart_printf("RIGHT pressed\n");
    if(btn & UP_LEFT) uart_printf("UP_LEFT pressed\n");
    if(btn & UP_RIGHT) uart_printf("UP_RIGHT pressed\n");
    if(btn & DOWN_LEFT) uart_printf("DOWN_LEFT pressed\n");
    if(btn & DOWN_RIGHT) uart_printf("DOWN_RIGHT pressed\n");
    if(btn & VOL_UP) uart_printf("VOL_UP pressed\n");
    if(btn & VOL_DOWN) uart_printf("VOL_DOWN pressed\n");
    if(btn & STICK) uart_printf("STICK pressed\n");
    if(btn & L) uart_printf("L pressed\n");
    if(btn & R) uart_printf("R pressed\n");*/
    
    if(btn & START) rgbSetFbAddress(fb0);
    else if(btn & SELECT) return 0;
    else rgbSetFbAddress(fb1);
  }
}

void transitionView(View* to) {
  if(currentView != NULL && currentView->deinit != NULL) {
    currentView->deinit();
  }
  
  if(to->init != NULL && to->init(errorMessage)) {
    displayError = true;
  } else {
    currentView = to;
    renderRequired = true;
  }
}

void clearError() {
  displayError = false;
}

void showError(const char* msg) {
  strcpy(errorMessage, msg);
  displayError = true;
  triggerRender();
}
