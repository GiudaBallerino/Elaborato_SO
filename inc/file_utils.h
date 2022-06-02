/// @file file_utils.h
/// @brief Contiene la definizioni di variabili e
///         funzioni specifiche per la gestione dei file

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "defines.h"
#include "err_exit.h"


struct file {
	char pathname[PATH_BUFFER_SIZE];
  struct file *next;
};

struct list {
  struct file *entry;
};

int openFile(char *pathname);
int getFileSize(char *pathname);
int getFileSizeFromFD(int fd);
size_t appendToPath(char *path, char *directory);
const char *getFilenameExt(const char *filename);

void append_file(char *pathname, struct list *l);
void reset_list(struct list *l);
void print_list(struct list *l);
