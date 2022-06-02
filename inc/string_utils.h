/// @file string_utils.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione delle string.

#pragma once

#include <string.h>

int strStartWith(const char *string, const char *prefix);
int strEndsWith(const char *string, const char *suffix);
char *strRemoveSuffix(char *string, const char* suffix);
