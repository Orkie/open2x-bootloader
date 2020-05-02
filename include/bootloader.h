#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__
#include <orcus.h>
#include <gp2xregs.h>
#include <gp2xtypes.h>

#include "background_bin.h"
#include "kernel.h"

#define VERSION "v1.0"

#define SIZEOF(a) sizeof(a)/sizeof(*a)

extern bool renderRequired;

#define triggerRender() {renderRequired = true;}

typedef struct {
  void (*render)(uint16_t*);
  void (*handleInput)(uint32_t, uint32_t);
} View;

extern View* currentView;

extern View MainMenu;

typedef struct {
  char* title;
  void (*callback)();
} MenuItem;

#define FONT_HEIGHT 12
#define FONT_WIDTH 6

typedef struct {
  int (*launch)(char* path);
} InternalInterpreter;

typedef struct {
  char* pathOfInterpreter;
} ExternalInterpreter;
  
union InterpreterDefinition {
  InternalInterpreter internal;
  ExternalInterpreter external;
};

typedef struct {
  char extension[3];
  bool isInternal;
  union InterpreterDefinition def;
} Interpreter;

#endif
