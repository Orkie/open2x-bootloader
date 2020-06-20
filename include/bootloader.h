#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__
#include <orcus.h>
#include <gp2xregs.h>
#include <gp2xtypes.h>

#include "background_bin.h"
#include "linux_bin.h"
#include "unknown_bin.h"
#include "folder_bin.h"
#include "kernel.h"

#define VERSION "v1.0"

#define SIZEOF(a) sizeof(a)/sizeof(*a)

extern bool renderRequired;

#define triggerRender() {renderRequired = true;}

typedef struct {
  void (*render)(uint16_t*);
  void (*handleInput)(uint32_t, uint32_t);
  int (*init)(char* errorMessage);
  void (*deinit)();
} View;

extern void transitionView(View* to);
extern void clearError();
extern void showError(const char* msg);
extern void blit(uint16_t* source, int sw, int sh, uint16_t* dest, int dx, int dy, int dw, int dh);

extern View MainMenu;
extern View Filer;

typedef struct {
  char* title;
  void (*callback)();
} MenuItem;

#define FONT_HEIGHT 12
#define FONT_WIDTH 6
#define RED 0xF800
#define WHITE 0xFFFF
#define BLACK 0x0000
#define MAGENTA 0xF81F

typedef struct {
  int (*launch)(char* path, char* arg);
  int (*getName)(char* path, char* output);
  int (*getIcon)(char* path, uint16_t* output);
} InternalInterpreter;

typedef struct {
  char* pathOfInterpreter;
} ExternalInterpreter;
  
union InterpreterDefinition {
  InternalInterpreter* internal;
  ExternalInterpreter* external;
};

typedef struct {
  char* extension;
  bool isInternal;
  union InterpreterDefinition def;
} Interpreter;

extern Interpreter O2xInterpreter;
extern Interpreter KernelInterpreter;

#define JMP(addr) \
    __asm__("mov pc,%0" \
            : /*output*/ \
            : /*input*/ \
            "r" (addr) \
           );

extern void terminalHandleChar(int c);
extern void terminalNewline();

#endif
