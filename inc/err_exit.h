/// @file err_exit.h
/// @brief Contiene la definizione della funzione di stampa degli errori.

#pragma once

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

/// @brief Prints the error message of the last failed
///         system call and terminates the calling process.
void errExit(const char *msg);
