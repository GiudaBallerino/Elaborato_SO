/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

#include <stddef.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>

#include "err_exit.h"

// definition of the union semun
union semun {
    int val;
    struct semid_ds * buf;
    unsigned short * array;
};

int semGet(int semKey, int sem_num);
void semOp(int semId, unsigned short sem_num, short sem_op);
int semOpNoWait(int semId, unsigned short sem_num, short sem_op);
void remove_semaphore(int semId);
int create_sem(key_t semKey, unsigned short value);
int create_sem_set(key_t semKey, int number, unsigned short values[]);
