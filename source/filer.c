#include <stdio.h>
#include <string.h>
#include <fat.h>
#include <stdlib.h>
#include <dirent.h>
#include "bootloader.h"

#define MAX_ON_SCREEN 9

typedef struct FileList {
  char name[NAME_MAX];
  bool isDir;
  struct FileList* next;
} FileList;

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
  // TODO sort such that directories are first, in alphabetical order, followed by files, in alphabetical order
  
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
    uart_printf("Couldn't open the directory\n");
    // TODO - report this error back to the user
  }
  
  free(path);

  currentDirectoryListing = out;
  return out;
}

static int startRenderingFrom = 0;
static int selected = 0;

static int init(char* error) {
  if(!fatInitDefault()) {
    strcpy(error, "No SD card detected.");
    return 1;
  }

  currentPath = newLink("sd:", true, NULL);
  updateCurrentDirectoryListing();

  selected = startRenderingFrom = 0;

  // TODO load interpreter list from SD and merge into builtin list
  
  return 0;
}

static void deinit() {
  freeList(currentPath);
}

static void render(uint16_t* fb) {
  int numberRendered = 0;
  FileList* next = nth(currentDirectoryListing, startRenderingFrom);

  char* path = pathFromList(currentPath->next, false);
  rgbPrintf(fb, 32, 32, 0x0000, "/%s", path);
  free(path);

  char buf[10];
  sprintf(buf, "%.3d / %.3d", selected > 998 || selected < 0 ? 0 : selected+1, currentDirectoryLength > 999 || currentDirectoryLength < 0 ? 0 : currentDirectoryLength);
  rgbPrintf(fb, 299-(FONT_WIDTH*9), 174-FONT_HEIGHT, BLACK, "%s", buf);

  while(next != NULL && numberRendered < MAX_ON_SCREEN) {
    int i = startRenderingFrom + numberRendered;
    
    if(selected == i) {
      rgbPrintf(fb, 32, 56+(FONT_HEIGHT*numberRendered), RED, "* ");
    }
    rgbPrintf(fb, 32+FONT_WIDTH*2, 56+(FONT_HEIGHT*numberRendered), selected == i ? RED : BLACK, next->name);
    
    numberRendered++;
    next = next->next;
  }
  // need a show/hide unsupported extentions feature
  // TODO - render instructions in bottom area
}

static void selectFile();

static void handleInput(uint32_t buttonStates, uint32_t buttonPresses) {
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
  
  // TODO - left/right should move a page at a time, keep selected the same relative to the page
  
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
    append(currentPath, newLink(selectedFile->name, true, NULL));
    
    // TODO - we need to look up the interpreter in ext->interp map, for now hardcode to o2x    
    O2xInterpreter.def.internal->launch(pathFromList(currentPath, false));

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
