/// @file message_queue.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della Message Queue.

#pragma once

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/msg.h>

#include "err_exit.h"

int create_message_queue(key_t msgKey);
int open_message_queue(key_t msgKey);
void remove_message_queue(int msqId);
