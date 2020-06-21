#include <stdio.h>
#include <string.h>
#include "bootloader.h"

#define SETTINGS_ADDR (0x80000-NAND_BLOCK_SIZE)
#define SETTINGS_MAGIC 0x7B88FB9D

BootloaderSettings currentSettings = {
				      SETTINGS_MAGIC,
				      false,
				      false
};

BootloaderSettings* loadSettings() {
  uint8_t buf[NAND_BLOCK_SIZE];
  nandRead(SETTINGS_ADDR, 1, buf);

  BootloaderSettings* settings = (BootloaderSettings*) buf;
  if(settings->magic != SETTINGS_MAGIC) {
    printf("Settings have not been set before, creating\n");
    saveSettings();
  } else {
    memcpy(&currentSettings, settings, sizeof(BootloaderSettings));
  }
  return &currentSettings;
}

void saveSettings() {
  uint8_t buf[NAND_BLOCK_SIZE];
  nandErase(SETTINGS_ADDR, 1);
  memset(buf, 0xff, NAND_BLOCK_SIZE);
  memcpy(buf, &currentSettings, sizeof(BootloaderSettings));
  nandWrite(SETTINGS_ADDR, 1, buf);
}

static int selected = 0;

static const MenuItem menu[] = {
  {.title = "Autoboot NAND", .onOff = &currentSettings.autobootNand},
  {.title = "Autoboot SD", .onOff = &currentSettings.autobootSd},
};

static void render(uint16_t* fb) {
  rgbPrintf(fb, (320-(8*FONT_WIDTH))>>1, 32, 0x0000, "Settings");

  for(int i = 0 ; i < SIZEOF(menu) ; i++) {
    if(selected == i) {
      rgbPrintf(fb, 32, 56+(FONT_HEIGHT*i), 0xF800, "* ");
    }
    rgbPrintf(fb, 32+FONT_WIDTH*2, 56+(FONT_HEIGHT*i), selected == i ? 0xF800 : 0x0000, menu[i].title);
    rgbPrintf(fb, 219, 56+(FONT_HEIGHT*i), 0x0000, *(menu[i].onOff) ? "On" : "Off");
  }
  rgbPrintf(fb, 32, 202, BLACK, "B: Toggle | X: Back");
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

  if(buttonPresses & B) {
    *(menu[selected].onOff) = !(*menu[selected].onOff);
    saveSettings();
    triggerRender();
  }

  if(buttonPresses & X) {
    transitionView(&MainMenu);
  }
}

View SettingsMenu = {
 &render,
 &handleInput,
 NULL,
 NULL
};
