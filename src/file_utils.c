/// @file file_utils.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei file

#include "file_utils.h"

int openFile(char *pathname) {
  int fd = open(pathname, O_RDONLY);
  if (fd == -1)
    errExit("open failed");
  return fd;
}

int getFileSize(char *pathname) {
  if (pathname == NULL)
    return 1;
  struct stat statbuf;
  if (stat(pathname, &statbuf) == -1)
    return 1;
  return statbuf.st_size;
}

int getFileSizeFromFD(int fd) {
  struct stat statbuf;
  if (fstat(fd, &statbuf) == -1)
    return 1;
  return statbuf.st_size;
}

size_t appendToPath(char *path, char *directory) {
  size_t length = strlen(path);
  strcat(strcat(path, "/"), directory);
  return length;
}

void append_file(char *pathname, struct list *l) {
  struct file *e = (struct file*) malloc(sizeof(struct file));
  strcpy(e->pathname, pathname);
  e->next = NULL;

  if (l->entry == NULL) {
    l->entry = e;
  } else {
    struct file *_ = l->entry;
    while (_->next != NULL) {
      _ = _->next;
    }
    _->next = e;
  }
}

void reset_list(struct list *l) {
  if (l->entry == NULL)
    return;

  while (l->entry != NULL) {
    struct file *head = l->entry;
    l->entry = head->next;
    free(head);
  }
}

void print_list(struct list *l) {
  printf("List:\n");
  struct file *_ = l->entry;
  while (_ != NULL) {
    printf("%s\n", _->pathname);
    _ = _->next;
  }
}
