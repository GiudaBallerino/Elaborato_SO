/// @file string_utils.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle String.

#include "string_utils.h"

int strStartWith(const char *string, const char *prefix) {
  return strncmp(string, prefix, strlen(prefix));
}

int strEndsWith(const char *string, const char *suffix) {
  if (!string || !suffix)
    return 0;
  size_t lenstr = strlen(string);
  size_t lensuffix = strlen(suffix);
  if (lensuffix >  lenstr)
    return 0;
  return strncmp(string + lenstr - lensuffix, suffix, lensuffix) == 0;
}

char *strRemoveSuffix(char *string, const char* suffix) {
  const int length = strlen(string);
  const int length_suffix = strlen(suffix);
  string[length- length_suffix] = '\0';
  return string;
}
