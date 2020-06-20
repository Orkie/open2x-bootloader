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
  setbuf(stdout, NULL);
  
  printf("\n\n**********************************\nOpen2x Bootloader %s\n**********************************\n", VERSION);
  printf("Auto load kernel: %s\n", autoloadKernel ? "on" : "off");
  printf("**********************************\n");

  if(autoloadKernel && !(btnState()&START)) {
    launchKernelFromNand();
    printf("Failed to load kernel from NAND!\n");
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
  
  uartSetEcho(true);
  terminalNewline();
  int gotChar = EOF;

  uint32_t lastTick = timerGet();

  while(1) {

    // terminal character handler
    while((gotChar = uartGetc(false)) != EOF) {
      terminalHandleChar(gotChar);
    }

    // how long was it since we last looped?
    int msSinceLastLoop = timerNsSince(lastTick, &lastTick)/1000000;

    // figure out which button events we want to raise
    uint32_t currentButtonState = btnStateDebounced();

    // deal with current state
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
