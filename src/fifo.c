/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include "fifo.h"

void makeFIFO(char *path) {
  if (mkfifo(path, S_IRUSR|S_IWUSR) == -1)
    errExit("mkfifo failed");
}

int openFIFO(char *path, int flag) {
  int f = open(path, flag);
  if (f == -1)
    errExit("open failed");
  return f;
}

void closeFIFO(int fifo) {
  if (fifo != -1 && close(fifo) != 0)
    errExit("close failed");
}

void unlinkFIFO(const char *path) {
  if (unlink(path) != 0)
    errExit("unlink failed");
}
