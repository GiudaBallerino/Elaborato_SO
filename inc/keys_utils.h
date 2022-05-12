/// @file keys_utils.h
/// @brief Contiene la definizioni di variabili e
///         funzioni specifiche per la gestione dei chiavi

#include <sys/ipc.h>
#include <sys/ipc.h>

#include "err_exit.h"

key_t get_key(const char *__path, const int __id);
