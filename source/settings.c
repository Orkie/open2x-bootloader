#include "bootloader.h"

#define SETTINGS_ADDR (0x80000-NAND_BLOCK_SIZE)
#define SETTINGS_MAGIC 0x7B88FB9D
typedef struct {
  uint32_t magic;
  bool autobootNand;
  bool autobootSd;
} BootloaderSettings;

BootloaderSettings currentSettings = {
				      SETTINGS_MAGIC,
				      false,
				      false
};

void loadSettings() {
  uint8_t buf[NAND_BLOCK_SIZE];
  nandRead(SETTINGS_ADDR, 1, buf);

  BootloaderSettings* settings = (BootloaderSettings*) buf;
  if(settings->magic != SETTINGS_MAGIC) {
    printf("Settings have not been set before, creating\n");
    nandErase(SETTINGS_ADDR, 1);
    memset(buf, 0xff, NAND_BLOCK_SIZE);
    memcpy(buf, &currentSettings, sizeof(BootloaderSettings));
    nandWrite(SETTINGS_ADDR, 1, buf);
  } else {
    memcpy(&currentSettings, settings, sizeof(BootloaderSettings));
  }
}
