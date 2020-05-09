#include <string.h>
#include <fat.h>
#include <stdlib.h>
#include <dirent.h>
#include "bootloader.h"

typedef struct FilenameLinkedList {
  char name[NAME_MAX];
  bool isDir;
  struct FilenameLinkedList* next;
} FilenameLinkedList;

static FilenameLinkedList* newLink(char* name, bool isDir, FilenameLinkedList* next) {
  FilenameLinkedList* out = malloc(sizeof(FilenameLinkedList));
  strcpy(out->name, name);
  out->isDir = isDir;
  out->next = next;
  return out;
}

static FilenameLinkedList* append(FilenameLinkedList* list, FilenameLinkedList* toAppend) {
  FilenameLinkedList* next = list;
  while(next != NULL) {
    if(next->next == NULL) {
      next->next = toAppend;
      return list;
    } else {
      next = next->next;
    }
  }
}

static void freeList(FilenameLinkedList* list) {
  FilenameLinkedList* next = list;
  while(next != NULL) {
    FilenameLinkedList* tmp = next->next;
    free(tmp);
    next = tmp;
  }
}

static char* pathFromList(FilenameLinkedList* list, bool isDirectory) {
  int totalStrLen = 0;
  int numberOfLinks = 0;
  FilenameLinkedList* next = list;
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

static FilenameLinkedList* currentPath;
static FilenameLinkedList* currentDirectoryListing = NULL;

static FilenameLinkedList* updateCurrentDirectoryListing() {
  if(currentDirectoryListing != NULL) {
    freeList(currentDirectoryListing);
  }
  
  DIR *dp;
  struct dirent *ep;
  char* path = pathFromList(currentPath, true);
  FilenameLinkedList* out = NULL;

  dp = opendir(path);
  if(dp != NULL) {
    while ((ep = readdir(dp))) {
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

static int init(char* error) {
  if(!fatInitDefault()) {
    strcpy(error, "No SD card detected.");
    return 1;
  }

  currentPath = newLink("sd:", true, NULL);
  updateCurrentDirectoryListing();
  
  return 0;
}

static void deinit() {
  freeList(currentPath);
}

static void render(uint16_t* fb) {
  FilenameLinkedList* next = currentDirectoryListing;
  while(next != NULL) {
    uart_printf("[%s] %s\n", (next->isDir ? "DIR" : "BIN"), next->name);
    next = next->next;
  }
}

static void handleInput(uint32_t buttonStates, uint32_t buttonPresses) {
  if(buttonPresses & X && currentPath->next == NULL) {
    transitionView(&MainMenu);
  }
}

View Filer = {
 &render,
 &handleInput,
 &init,
 &deinit
};
