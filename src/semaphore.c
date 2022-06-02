/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "semaphore.h"

void printSemaphoresValue(int semid) {
  unsigned short semVal[4];
  union semun arg;
  arg.array = semVal;
  if (semctl(semid, 0 /*ignored*/, GETALL, arg) == -1)
      errExit("semctl GETALL failed");
  printf("semaphore set state:\n");
  for (int i = 0; i < 4; i++)
      printf("id: %d --> %d\n", i, semVal[i]);
}

int semGet(int semKey, int sem_num) {
  int semId = semget(semKey, sem_num, S_IRUSR | S_IWUSR);
  if (semId == -1)
    errExit("semget failed\n");
  return semId;
}

void semOp(int semId, unsigned short sem_num, short sem_op) {
  struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};
  if (semop(semId, &sop, 1) == -1){
    printf("semop failed: %i %i %i\n", semId, sem_num, sem_op);
    printSemaphoresValue(semId);
    errExit("semop failed");
  }
}

int semOpNoWait(int semId, unsigned short sem_num, short sem_op) {
  struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = IPC_NOWAIT};
  return semop(semId, &sop, 1);
}

void remove_semaphore(int semId) {
  if (semctl(semId, 0 /*ignored*/, IPC_RMID, NULL) == -1)
    errExit("semctl IPC_RMID failed");
}

int create_sem(key_t semKey, unsigned short value) {
  int semId = semget(semKey, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
  if (semId == -1)
    errExit("semget failed");

  union semun arg;
  arg.val = value;

  if (semctl(semId, 0 /*ignored*/, SETVAL, arg) == -1)
    errExit("semctl SETALL failed");
  return semId;
}

int create_sem_set(key_t semKey, int sem_num, unsigned short values[]) {
  int semId = semget(semKey, sem_num, IPC_CREAT | S_IRUSR | S_IWUSR);
  if (semId == -1)
    errExit("semget failed");

  union semun arg;
  arg.array = values;

  if (semctl(semId, 0 /*ignored*/, SETALL, arg) == -1)
    errExit("semctl SETALL failed");
  return semId;
}
