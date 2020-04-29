#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__
#include <orcus.h>
#include <gp2xregs.h>
#include <gp2xtypes.h>

#include "font_bin.h"
#include "kernel.h"

extern bool renderRequired;

#define triggerRender() {renderRequired = true;}

typedef struct {
  void (*render)(uint16_t*);
  void (*handleInput)(uint32_t, uint32_t);
} View;

extern View* currentView;

extern View MainMenu;

#endif
