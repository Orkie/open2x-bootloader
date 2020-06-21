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

struct ButtonHeldTime {
  int A_held;
  int B_held;
  int X_held;
  int Y_held;
  int START_held;
  int SELECT_held;
  int UP_held;
  int DOWN_held;
  int LEFT_held;
  int RIGHT_held;
  int UP_LEFT_held;
  int UP_RIGHT_held;
  int DOWN_LEFT_held;
  int DOWN_RIGHT_held;
  int VOL_UP_held;
  int VOL_DOWN_held;
  int STICK_held;
  int L_held;
  int R_held;
};

#define REPEAT_INITIAL_MS 400
#define REPEAT_MS 50

uint32_t decideNewPresses(uint32_t current, uint32_t last, struct ButtonHeldTime* heldTime, int msSinceLastLoop) {

  heldTime->A_held = (current & A) ? heldTime->A_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->B_held = (current & B) ? heldTime->B_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->X_held = (current & X) ? heldTime->X_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->Y_held = (current & Y) ? heldTime->Y_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->START_held = (current & START) ? heldTime->START_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->SELECT_held = (current & SELECT) ? heldTime->SELECT_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->UP_held = (current & UP) ? heldTime->UP_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->DOWN_held = (current & DOWN) ? heldTime->DOWN_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->LEFT_held = (current & LEFT) ? heldTime->LEFT_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->RIGHT_held = (current & RIGHT) ? heldTime->RIGHT_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->UP_LEFT_held = (current & UP_LEFT) ? heldTime->UP_LEFT_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->UP_RIGHT_held = (current & UP) ? heldTime->UP_RIGHT_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->DOWN_LEFT_held = (current & DOWN_LEFT) ? heldTime->DOWN_LEFT_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->DOWN_RIGHT_held = (current & DOWN_RIGHT) ? heldTime->DOWN_RIGHT_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->VOL_UP_held = (current & VOL_UP) ? heldTime->VOL_UP_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->VOL_DOWN_held = (current & VOL_DOWN) ? heldTime->VOL_DOWN_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->STICK_held = (current & STICK) ? heldTime->STICK_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->L_held = (current & L) ? heldTime->L_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  heldTime->R_held = (current & R) ? heldTime->R_held - msSinceLastLoop : REPEAT_INITIAL_MS;
  
  uint32_t repeatPresses = 0;
  if(heldTime->A_held <= 0) {
    heldTime->A_held += REPEAT_MS;
    repeatPresses |= A;
  }
  if(heldTime->B_held <= 0) {
    heldTime->B_held += REPEAT_MS;
    repeatPresses |= B;
  }
  if(heldTime->X_held <= 0) {
    heldTime->X_held += REPEAT_MS;
    repeatPresses |= X;
  }
  if(heldTime->Y_held <= 0) {
    heldTime->Y_held += REPEAT_MS;
    repeatPresses |= Y;
  }
  if(heldTime->START_held <= 0) {
    heldTime->START_held += REPEAT_MS;
    repeatPresses |= START;
  }
  if(heldTime->SELECT_held <= 0) {
    heldTime->SELECT_held += REPEAT_MS;
    repeatPresses |= SELECT;
  }
  if(heldTime->UP_held <= 0) {
    heldTime->UP_held += REPEAT_MS;
    repeatPresses |= UP;
  }
  if(heldTime->DOWN_held <= 0) {
    heldTime->DOWN_held += REPEAT_MS;
    repeatPresses |= DOWN;
  }
  if(heldTime->LEFT_held <= 0) {
    heldTime->LEFT_held += REPEAT_MS;
    repeatPresses |= LEFT;
  }
  if(heldTime->RIGHT_held <= 0) {
    heldTime->RIGHT_held += REPEAT_MS;
    repeatPresses |= RIGHT;
  }
  if(heldTime->UP_LEFT_held <= 0) {
    heldTime->UP_LEFT_held += REPEAT_MS;
    repeatPresses |= UP_LEFT;
  }
  if(heldTime->UP_RIGHT_held <= 0) {
    heldTime->UP_RIGHT_held += REPEAT_MS;
    repeatPresses |= UP_RIGHT;
  }
  if(heldTime->DOWN_LEFT_held <= 0) {
    heldTime->DOWN_LEFT_held += REPEAT_MS;
    repeatPresses |= DOWN_LEFT;
  }
  if(heldTime->DOWN_RIGHT_held <= 0) {
    heldTime->DOWN_RIGHT_held += REPEAT_MS;
    repeatPresses |= DOWN_RIGHT;
  }
  if(heldTime->VOL_UP_held <= 0) {
    heldTime->VOL_UP_held += REPEAT_MS;
    repeatPresses |= VOL_UP;
  }
  if(heldTime->VOL_DOWN_held <= 0) {
    heldTime->VOL_DOWN_held += REPEAT_MS;
    repeatPresses |= VOL_DOWN;
  }
  if(heldTime->STICK_held <= 0) {
    heldTime->STICK_held += REPEAT_MS;
    repeatPresses |= STICK;
  }
  if(heldTime->L_held <= 0) {
    heldTime->L_held += REPEAT_MS;
    repeatPresses |= L;
  }
  if(heldTime->R_held <= 0) {
    heldTime->R_held += REPEAT_MS;
    repeatPresses |= R;
  }
  
  return (current&(~last)) | repeatPresses;
}

int main() {
  gp2xInit();
  setbuf(stdout, NULL);

  if((btnState()&L) && (btnState()&R)) {
    saveSettings();
    printf("\nSettings reset\n");
    showError("Settings were reset");
  }
  
  loadSettings();
  
  printf("\n**********************************\nOpen2x Bootloader %s\n**********************************\n", VERSION);
  printf("Auto boot NAND: %s\n", currentSettings.autobootNand ? "on" : "off");
  printf("Auto boot SD: %s\n", currentSettings.autobootSd ? "on" : "off");
  printf("**********************************\n");
  
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

  // TODO - look at brief screen corruption if file is missing
  if(currentSettings.autobootNand && !(btnState()&R)) {
    launchKernelFromNand();
    showError("Failed to launch kernel");
  }

  if(currentSettings.autobootSd && !(btnState()&R)) {
    if(fatInitDefault()) {
      O2xInterpreter.def.internal->launch("sd:/autoboot.o2x", NULL);
      fatUnmount("sd");      
    }
  }  
  
  transitionView(&MainMenu);

  uint16_t* currentFb = fb0;
  uint16_t* nextFb = fb1;
  uint32_t previousButtonState = 0;
  
  uartSetEcho(true);
  terminalNewline();
  int gotChar = EOF;

  uint32_t lastTick = timerGet();
  struct ButtonHeldTime heldTime = { REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS,
				     REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS,
				     REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS, REPEAT_INITIAL_MS,
				     REPEAT_INITIAL_MS};
  
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
      currentView->handleInput(currentButtonState, decideNewPresses(currentButtonState, previousButtonState, &heldTime, msSinceLastLoop));
    }
    previousButtonState = currentButtonState;

    if(renderRequired) {
      memcpy((void*) nextFb, (void*) background_bin, 320*240*2);
      if(currentView != NULL) {
	currentView->render(nextFb);
      }

      if(displayError) {	
	rgbPrintfBg(nextFb,
		    20,
		    224,
		    WHITE,
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
    renderRequired = true;
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

void blit(uint16_t* source, int sw, int sh, uint16_t* dest, int dx, int dy, int dw, int dh) {
  for(int y = sh ; y-- ; ) {
    for(int x = sw ; x-- ; ) {
      uint16_t pixel = *(source + (sw*y) + x);
      if(pixel != MAGENTA) {
	*(dest + ((dy+y)*dw) + (dx+x)) = pixel;
      }
    }
  }
}

void drawBox(uint16_t* dest, int dw, int x, int y, int w, int h, uint16_t colour) {
  for(int cy = h ; cy-- ; ) {
    for(int cx = w ; cx-- ; ) {
      *(dest + ((y+cy)*dw) + (x+cx)) = colour;
    }
  }
}

void drawBoxOutline(uint16_t* dest, int dw, int x, int y, int w, int h, uint16_t colour) {
  for(int cy = h ; cy-- ; ) {
    for(int cx = w ; cx-- ; ) {
      if(cy == 0 || cy == (h-1) || cx == 0 || cx == (w-1)) {
	*(dest + ((y+cy)*dw) + (x+cx)) = colour;
      }
    }
  }
}

