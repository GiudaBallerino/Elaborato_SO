/// @file fifo.h
/// @brief Contiene la definizioni di variabili e
///         funzioni specifiche per la gestione delle FIFO.

#pragma once

#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "err_exit.h"

void makeFIFO(char *path);
int openFIFO(char *path, int flag);
void closeFIFO(int fifo);
void unlinkFIFO(const char *fifo);
