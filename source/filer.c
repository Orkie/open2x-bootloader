#include <stdio.h>
#include <string.h>
#include <fat.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include "bootloader.h"

#define MAX_ON_SCREEN 9

typedef struct FileList {
  char name[NAME_MAX];
  bool isDir;
  struct FileList* next;
} FileList;

static int length(FileList* list) {
  int i = 1;
  FileList* current = list;
  while((current = current->next) != NULL) {    
    i++;
  }
  return i;
}

static int compare(FileList* a, FileList* b) {
  if(a->isDir && !b->isDir) {
    return -1;
  } else if(!a->isDir && b->isDir) {
    return 1;
  } else {
    return strcmp(a->name, b->name);
  }
}

static FileList* sort(FileList* list) {
  int len = length(list);
  FileList* files[len];

  FileList* current = list;
  for(int i = 0 ; i < len ; i++) {
    files[i] = current;
    current = current->next;
  }
  
  // bubble sort is simple but slow, hopefully it won't really matter for what we're doing
  for(int endOfScan = len  ; endOfScan > 0 ; endOfScan--) {
    for(int currentPairStart = 0 ; currentPairStart < endOfScan - 1 ; currentPairStart++) {
      FileList* a = files[currentPairStart];
      FileList* b = files[currentPairStart+1];
      if(compare(a, b) > 0) {
	files[currentPairStart] = b;
	files[currentPairStart+1] = a;
      }
    }
  }
  
  current = NULL;  
  for(int i = len ; i-- ; ) {
    files[i]->next = current;
    current = files[i];
  }
  
  return current;
}

static FileList* nth(FileList* list, int nth) {
  int n = 0;
  FileList* next = list;
  while(n < nth) {
    if(next->next == NULL) {
      return NULL;
    }
    next = next->next;
    n++;
  }
  return next;
}

static FileList* newLink(char* name, bool isDir, FileList* next) {
  FileList* out = malloc(sizeof(FileList));
  strcpy(out->name, name);
  out->isDir = isDir;
  out->next = next;
  return out;
}

static FileList* append(FileList* list, FileList* toAppend) {
  FileList* next = list;
  while(next != NULL) {
    if(next->next == NULL) {
      next->next = toAppend;
      return list;
    } else {
      next = next->next;
    }
  }
  return toAppend;
}

static FileList* pop(FileList* list) {
  FileList* next = list;
  while(next != NULL) {
    if(next->next != NULL && next->next->next == NULL) {
      FileList* out = next->next;
      next->next = NULL;
      return out;
    }
    next = next->next;
  }
  return NULL;
}

static void freeList(FileList* list) {
  FileList* next = list;
  while(next != NULL) {
    FileList* tmp = next->next;
    free(next);
    next = tmp;
  }
}

static char* pathFromList(FileList* list, bool isDirectory) {
  int totalStrLen = 0;
  int numberOfLinks = 0;
  FileList* next = list;
  while(next != NULL) {
    totalStrLen += strlen(next->name);
    numberOfLinks++; 
    next = next->next;
  }

  char* out = malloc(totalStrLen + (numberOfLinks - 1) + (isDirectory ? 1 : 0) + 1); // length of characters + length of dividing /s + termination
  char* currentOut = out;
  next = list;
  while(next != NULL) {
    strcpy(currentOut, next->name);
    currentOut += strlen(next->name);

    if(next->next != NULL) {
      strcpy(currentOut, "/");
      currentOut += 1;
    }
    
    next = next->next;
  }

  if(isDirectory) {
    *(currentOut) = '/';
    currentOut += 1;
  }
  
  *(currentOut) = '\0';
  return out;
}

static FileList* currentPath;
static FileList* currentDirectoryListing = NULL;
static int currentDirectoryLength = 0;

static FileList* updateCurrentDirectoryListing() {
  if(currentDirectoryListing != NULL) {
    freeList(currentDirectoryListing);
  }
  currentDirectoryLength = 0;
  
  DIR *dp;
  struct dirent *ep;
  char* path = pathFromList(currentPath, true);
  FileList* out = NULL;

  dp = opendir(path);
  if(dp != NULL) {
    while ((ep = readdir(dp))) {
      if(strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) {
	continue;
      }
      currentDirectoryLength++;
      out = newLink(ep->d_name, ep->d_type == DT_DIR, out);
    }
    closedir(dp);
  } else {
    uartPrintf("Couldn't open the directory\n");
    showError("Couldn't open directory");
  }
  
  free(path);

  currentDirectoryListing = sort(out);
  return out;
}

static int startRenderingFrom = 0;
static int selected = 0;
static bool canFlash = false;
static bool seekConfirmation = false;
static bool flashing = false;
static bool performRealFlash = false;

#define INTERPRETERS_MAX 130
#define PREDEFINED_INTERPRETERS 2
static Interpreter* interpreters[INTERPRETERS_MAX];

static void loadInterpretersIni() {
  FILE* fp = fopen("sd:/interpreters.ini", "r");
  if(fp == NULL || ferror(fp)) {
    return;
  }

  char line[1024];
  int linesRead = 0;
  int currentInterpreter = PREDEFINED_INTERPRETERS;

  while(fgets(line, 1024, fp) != NULL && (linesRead++ < (INTERPRETERS_MAX-2))) {
    Interpreter* newInterp = malloc(sizeof(Interpreter));
    if(newInterp == NULL) {
      uartPrintf("Can't allocate space for interpreter\n");
      return;
    }

    ExternalInterpreter* newExtInterp = malloc(sizeof(ExternalInterpreter));
    if(newExtInterp == NULL) {
      uartPrintf("Can't allocate space for interpreter\n");
      free(newInterp);
      return;
    }

    char* equals = strchr(line, '=');
    if(equals == NULL) {
      uartPrintf("Line does not contain equals sign: %s\n", line);
      free(newInterp);
      free(newExtInterp);
      return;
    }
    int extensionLength = (int) (equals - line);
    char* extension = malloc(extensionLength+1);
    if(extension == NULL) {
      uartPrintf("Couldn't allocate space for ext\n");
      free(newInterp);
      free(newExtInterp);
      return;
    }

    strncpy(extension, line, extensionLength+1);
    extension[extensionLength] = '\0';
    
    newInterp->extension = extension;
    newInterp->isInternal = false;
    newInterp->def.external = newExtInterp;

    newExtInterp->pathOfInterpreter = malloc(strlen(equals+1)+1);
    if(newExtInterp->pathOfInterpreter == NULL) {
      uartPrintf("Couldn't allocate space for ext\n");
      free(newInterp);
      free(newExtInterp);
      free(extension);
      return;
    }

    strcpy(newExtInterp->pathOfInterpreter, equals+1);
    char* newline = strrchr(newExtInterp->pathOfInterpreter, '\n');
    if(newline != NULL) {
      *newline = '\0';
    }
    
    interpreters[currentInterpreter++] = newInterp;
  }

  fclose(fp);
}

static char* decideExtension(FileList* file) {
  char* selectedFileExtension = strrchr(file->name, '.');
  if(selectedFileExtension == NULL) {
    return NULL;
  }
  selectedFileExtension++;
  return selectedFileExtension;
}

static Interpreter* findInterpreter(FileList* file) {
  char* extension = decideExtension(file);
  if(extension == NULL) {
    return NULL;
  }

  char copiedExtn[strlen(extension)];
  strcpy(copiedExtn, extension);
  
  char* currentChar = copiedExtn;
  while(*currentChar != '\0') {
    *currentChar = tolower(*currentChar);
    currentChar++;
  }

  Interpreter* interpreter = NULL;
  for(int i = INTERPRETERS_MAX ; i-- ; ) { // done from the end so you can override the builtins
    if(interpreters[i] != NULL && strcmp(copiedExtn, interpreters[i]->extension) == 0) {
      interpreter = interpreters[i];
      break;
    }
  }

  if(interpreter == NULL) {
    return NULL;
  }

  return interpreter;
}

void doRunFile(char* extension, char* path) {
  char* currentChar = extension;
  while(*currentChar != '\0') {
    *currentChar = tolower(*currentChar);
    currentChar++;
  }

  Interpreter* interpreter = NULL;
  for(int i = INTERPRETERS_MAX ; i-- ; ) { // done from the end so you can override the builtins
    if(interpreters[i] != NULL && strcmp(extension, interpreters[i]->extension) == 0) {
      interpreter = interpreters[i];
      break;
    }
  }

  if(interpreter == NULL) {
    showError("No interpreter");
    return;
  }
    
  if(interpreter->isInternal) {
    interpreter->def.internal->launch(path, NULL);
  } else {
    O2xInterpreter.def.internal->launch(interpreter->def.external->pathOfInterpreter,
					path);
  }
}

static int init(char* error) {
  if(!fatInitDefault()) {
    strcpy(error, "No SD card detected.");
    return 1;
  }

  currentPath = newLink("sd:", true, NULL);
  updateCurrentDirectoryListing();

  selected = startRenderingFrom = 0;

  for(int i = INTERPRETERS_MAX ; i >= PREDEFINED_INTERPRETERS ; i--) {
    if(interpreters[i] != NULL) {
      free(interpreters[i]->def.external->pathOfInterpreter);
      free(interpreters[i]);
    }
    interpreters[i] = NULL;
  }
  interpreters[0] = &O2xInterpreter;
  interpreters[1] = &KernelInterpreter;
  loadInterpretersIni();

  return 0;
}

static void deinit() {
  freeList(currentPath);
  currentPath = NULL;
  freeList(currentDirectoryListing);
  currentDirectoryListing = NULL;
  fatUnmount("sd");
}

static int handleSelected(FileList* selected, uint16_t* icon, char* name) {

  strcpy(name, "Unknown file type");
  memcpy(icon, unknown_bin, 16*16*2);

  if(selected->isDir) {
    strcpy(name, "Directory");
    memcpy(icon, folder_bin, 16*16*2);
    return 0;
  }

  Interpreter* interpreter = findInterpreter(selected);
  if(interpreter == NULL) {
    return 1;
  }
  
  append(currentPath, newLink(selected->name, true, NULL));
  char* path = pathFromList(currentPath, false);
  pop(currentPath);
  
  if(interpreter->isInternal) {
    int nameRes = interpreter->def.internal->getName(path, name);
    int iconRes = interpreter->def.internal->getIcon(path, icon);
    if(interpreter == &KernelInterpreter && nameRes == 0 && iconRes == 0) {
      canFlash = true;
    }
  } else {
    O2xInterpreter.def.internal->getName(interpreter->def.external->pathOfInterpreter, name);
    O2xInterpreter.def.internal->getIcon(interpreter->def.external->pathOfInterpreter, icon);    
  }
  return 0;
}

static void render(uint16_t* fb) {
  int numberRendered = 0;
  FileList* next = nth(currentDirectoryListing, startRenderingFrom);

  char* path = pathFromList(currentPath->next, false);
  rgbPrintf(fb, 32, 32, 0x0000, "/%s", path);
  free(path);

  uint16_t icon[16*16];
  char name[32];
  canFlash = false;
  memset(name, '\0', 32);

  char buf[10];
  sprintf(buf, "%.3d / %.3d", selected > 998 || selected < 0 ? 0 : selected+1, currentDirectoryLength > 999 || currentDirectoryLength < 0 ? 0 : currentDirectoryLength);
  rgbPrintf(fb, 299-(FONT_WIDTH*9), 174-FONT_HEIGHT, BLACK, "%s", buf);

  while(next != NULL && numberRendered < MAX_ON_SCREEN) {
    int i = startRenderingFrom + numberRendered;
    
    if(selected == i) {
      rgbPrintf(fb, 32, 56+(FONT_HEIGHT*numberRendered), RED, "* ");
      handleSelected(next, icon, name);
    }
    rgbPrintf(fb, 32+FONT_WIDTH*2, 56+(FONT_HEIGHT*numberRendered), selected == i ? RED : BLACK, (next->isDir ? "[%s]" : "%s"), next->name);
    
    numberRendered++;
    next = next->next;
  }

  blit(icon, 16, 16, fb, 32, 182, 320, 240);
  rgbPrintf(fb, 32+16+8, 184, BLACK, "%s", name);

  rgbPrintf(fb, 32, 202, BLACK, (canFlash ? "B: Select | X: Back | Y: Flash" : "B: Select | X: Back"));
  
  if(seekConfirmation) {
    drawBox(fb, 320, 64, 88, 192, 64, WHITE);
    drawBoxOutline(fb, 320, 64, 88, 192, 64, 0x8410);
    rgbPrintf(fb, 64+((192>>1)-((FONT_WIDTH*13)>>1)), 88+4, RED, "Are you sure?");
    rgbPrintf(fb, 64+4, 88+4+FONT_HEIGHT, BLACK, "Please confirm you wish to\nflash this kernel");
    rgbPrintf(fb, 64+((192>>1)-((FONT_WIDTH*14)>>1)), 88+64-FONT_HEIGHT-4, BLACK, "B: Yes | X: No");
  }

  if(flashing) {
    drawBox(fb, 320, 64, 88, 192, 64, WHITE);
    drawBoxOutline(fb, 320, 64, 88, 192, 64, 0x8410);
    rgbPrintf(fb, 64+((192>>1)-((FONT_WIDTH*8)>>1)), 88+4, RED, "Flashing");
    performRealFlash = true;
    flashing = false;
  }
}

static void selectFile();

static void handleInputBase(uint32_t buttonStates, uint32_t buttonPresses) {
  if(buttonPresses & DOWN && selected < (currentDirectoryLength-1)) {
    selected++;
    if(selected >= MAX_ON_SCREEN) {
      startRenderingFrom++;
    }
    triggerRender();
  }

  if(buttonPresses & UP && selected > 0) {
    selected--;
    if(selected < startRenderingFrom) {
      startRenderingFrom--;
    }
    triggerRender();
  }

  if(buttonPresses & B) {
    selectFile();
  }

  if(buttonPresses & LEFT) {
    if(selected < MAX_ON_SCREEN) {
      selected = 0;
    } else {
      selected -= MAX_ON_SCREEN;
      startRenderingFrom = (startRenderingFrom - MAX_ON_SCREEN) < 0 ? 0 : startRenderingFrom - MAX_ON_SCREEN;
    }
    triggerRender();
  }

  if(buttonPresses & RIGHT) {
    if((selected + MAX_ON_SCREEN) >= currentDirectoryLength) {
      selected = currentDirectoryLength-1;
      startRenderingFrom = (selected - MAX_ON_SCREEN) < 0 ? 0 : (selected - MAX_ON_SCREEN + 1);
    } else {
      selected += MAX_ON_SCREEN;
      startRenderingFrom += MAX_ON_SCREEN;
    }
    triggerRender();
  }
  
  if(buttonPresses & X && currentPath->next == NULL) {
    transitionView(&MainMenu);
  } else if(buttonPresses & X) {
    FileList* last = pop(currentPath);

    updateCurrentDirectoryListing();

    FileList* next = currentDirectoryListing;
    int i = 0;
    selected = startRenderingFrom = 0;
    while(next != NULL) {
      if(strcmp(last->name, next->name) == 0) {
	selected = i;
	startRenderingFrom = selected < MAX_ON_SCREEN ? 0 : selected;
	break;
      }
      next = next->next;
      i++;
    }
    
    if(last != NULL) {
      free(last);
    }
    triggerRender();
  }

  if(buttonPresses & Y && canFlash) {
    seekConfirmation = true;
    triggerRender();
  }
}

static void handleInputConfirmFlash(uint32_t buttonStates, uint32_t buttonPresses) {
  if(buttonPresses & X) {
    seekConfirmation = false;
    triggerRender();
    return;
  }

  if(buttonPresses & B) {    
    seekConfirmation = false;
    flashing = true;
    triggerRender();
  }
}

static void handleInput(uint32_t buttonStates, uint32_t buttonPresses) {
  if(buttonPresses) {
    clearError();
  }

  if(performRealFlash) {
    FileList* selectedFile = nth(currentDirectoryListing, selected);
    if(selectedFile == NULL) {
      return;
    }

    append(currentPath, newLink(selectedFile->name, true, NULL));
    char* path = pathFromList(currentPath, false);
    pop(currentPath);

    flashKernelFromFile(path);
    performRealFlash = false;
    triggerRender();
    return;
  }

  if(seekConfirmation) {
    handleInputConfirmFlash(buttonStates, buttonPresses);
  } else {
    handleInputBase(buttonStates, buttonPresses);
  }
  
}

static void selectFile() {
  FileList* selectedFile = nth(currentDirectoryListing, selected);
  if(selectedFile == NULL) {
    return;
  }

  if(selectedFile->isDir) {
    append(currentPath, newLink(selectedFile->name, true, NULL));
    updateCurrentDirectoryListing();
    selected = startRenderingFrom = 0;
    triggerRender();
  } else {
    char* selectedFileExtension = strrchr(selectedFile->name, '.');
    if(selectedFileExtension == NULL) {
      showError("File has no extension");
      return;
    }
    selectedFileExtension++;

    append(currentPath, newLink(selectedFile->name, true, NULL));
    char* path = pathFromList(currentPath, false);

    doRunFile(selectedFileExtension, path);
    
    // failed
    pop(currentPath);
  }
}

View Filer = {
 &render,
 &handleInput,
 &init,
 &deinit
};
