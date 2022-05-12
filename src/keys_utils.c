/// @file keys_utils.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle 

#include "keys_utils.h"

key_t get_key(const char *__path, const int __id) {
  key_t shmKey;
  if ((shmKey = ftok("/tmp", 'a')) == (key_t) -1) {
    errExit("IPC error: ftok");
  }
  return shmKey;
}
