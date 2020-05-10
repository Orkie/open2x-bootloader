#include "bootloader.h"

#define O2X_MAGIC 0x6F327831

typedef struct {
  uint32_t length;
  uint32_t loadAddress;
} O2xSection;

typedef struct {
  uint32_t magic;
  char name[32];
  uint16_t icon[16*16];
  uint8_t reserved[8];
  uint32_t paramLength;
  uint32_t paramAddr;
  uint8_t numberOfSections;
  O2xSection* sections;
} O2xHeader;

static int getName(char* name) {
}

static int launch(char* path) {
}

InternalInterpreter O2xInternalInterpreter = {
  &launch,
  &getName
};

Interpreter O2xInterpreter = {
 "o2x",
 true,
 {.internal = &O2xInternalInterpreter}
};
