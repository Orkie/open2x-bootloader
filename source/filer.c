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
    uartPrintf("Couldn't open the directory\n");
    // TODO - report this error back to the user
  }
  
  free(path);

  currentDirectoryListing = out;
  return out;
}

static int startRenderingFrom = 0;
static int selected = 0;

#define INTERPRETERS_MAX 130
static Interpreter* interpreters[INTERPRETERS_MAX];

static void loadInterpretersIni() {
  FILE* fp = fopen("sd:/interpreters.ini", "r");
  if(fp == NULL || ferror(fp)) {
    uartPrintf("No interpreters file on SD\n");
    return;
  }

  char line[1024];

  int linesRead = 0;
  int currentInterpreter = 2;

  uartPrintf("1\n");

  while(fgets(line, 1024, fp) != NULL && (linesRead++ < (INTERPRETERS_MAX-2))) {
    uartPrintf("This is a line: %s\n", line);
  }
  fclose(fp);
  return;
  
  while(fgets(line, 1024, fp) != NULL && linesRead++ < (INTERPRETERS_MAX-2)) {
    uartPrintf("read line '%s'\n");
  uartPrintf("2\n");

    Interpreter* newInterp = malloc(sizeof(Interpreter));
    if(newInterp == NULL) {
      uartPrintf("Can't allocate space for interpreter\n");
      return;
    }
      uartPrintf("3\n");

    ExternalInterpreter* newExtInterp = malloc(sizeof(ExternalInterpreter));
    if(newExtInterp == NULL) {
      uartPrintf("Can't allocate space for interpreter\n");
      free(newInterp);
      return;
    }

      uartPrintf("4\n");

    char* equals = strchr(line, '=');
    if(equals == NULL) {
      uartPrintf("Line does not contain equals sign: %s\n", line);
      free(newInterp);
      free(newExtInterp);
      return;
    }
    int extensionLength = (int) (equals - line);

      uartPrintf("5\n");

    char* extension = malloc(extensionLength+1);
    if(extension == NULL) {
      uartPrintf("Couldn't allocate space for ext\n");
      free(newInterp);
      free(newExtInterp);
      return;
    }

      uartPrintf("6\n");


    strncpy(extension, line, extensionLength+1);
    extension[extensionLength] = '\0';
    
    newInterp->extension = extension;
    newInterp->isInternal = false;
    newInterp->def.external = newExtInterp;

      uartPrintf("7\n");

    newExtInterp->pathOfInterpreter = malloc(strlen(equals+1)+1);
    if(newExtInterp->pathOfInterpreter == NULL) {
      uartPrintf("Couldn't allocate space for ext\n");
      free(newInterp);
      free(newExtInterp);
      free(extension);
      return;
    }
      uartPrintf("8\n");

    strcpy(newExtInterp->pathOfInterpreter, equals+1);
    char* newline = strrchr(newExtInterp->pathOfInterpreter, '\n');
    if(newline != NULL) {
      *newline = '\0';
    }
      uartPrintf("9\n");

    
    interpreters[currentInterpreter++] = newInterp;


      uartPrintf("10\n");

  }
    uartPrintf("11\n");

  for(int i = INTERPRETERS_MAX ; i-- ;) {
    if(interpreters[i] != NULL) {
      uartPrintf("Interpreter for %s loaded\n", interpreters[i]->extension);
    }
  }
    uartPrintf("12\n");


  fclose(fp);
    uartPrintf("13\n");

}

static int init(char* error) {
  if(!fatInitDefault()) {
    strcpy(error, "No SD card detected.");
    return 1;
  }

  currentPath = newLink("sd:", true, NULL);
  updateCurrentDirectoryListing();

  selected = startRenderingFrom = 0;

  for(int i = INTERPRETERS_MAX ; i-- ; ) {
    if(interpreters[i] != NULL) {
      free(interpreters[i]); // TODO - free this fully
    }
    interpreters[i] = NULL;
  }
  interpreters[0] = &O2xInterpreter;
  interpreters[1] = &KernelInterpreter;
  //  loadInterpretersIni();
  
  return 0;
}

static void deinit() {
  freeList(currentPath);
  freeList(currentDirectoryListing);
  currentDirectoryListing = NULL;
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

    char* currentChar = selectedFileExtension;
    while(*currentChar != '\0') {
      *currentChar = tolower(*currentChar);
      currentChar++;
    }

    Interpreter* interpreter = NULL;
    for(int i = INTERPRETERS_MAX ; i-- ; ) { // done from the end so you can override the builtins
      if(interpreters[i] != NULL && strcmp(selectedFileExtension, interpreters[i]->extension) == 0) {
	interpreter = interpreters[i];
	break;
      }
    }

    if(interpreter == NULL) {
      showError("No interpreter");
      return;
    }
    
    append(currentPath, newLink(selectedFile->name, true, NULL));

    char* path = pathFromList(currentPath, false);
    if(interpreter->isInternal) {
      interpreter->def.internal->launch(path, NULL);
    } else {
      O2xInterpreter.def.internal->launch(interpreter->def.external->pathOfInterpreter,
					  path);
    }
    
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
