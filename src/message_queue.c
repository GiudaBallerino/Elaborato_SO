/// @file message_queue.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione della Message Queue.

#include "message_queue.h"

int create_message_queue(key_t msgKey) {
  int msqId = msgget(msgKey, IPC_CREAT | S_IRUSR | S_IWUSR);
  if (msqId == -1)
    errExit("msgget failed");
  return msqId;
}

int open_message_queue(key_t msgKey) {
  int msqId = msgget(msgKey, S_IRUSR | S_IWUSR);
  if (msqId == -1)
    errExit("msgget open failed");
  return msqId;
}

void remove_message_queue(int msqId) {
  if (msqId != -1 && msgctl(msqId, IPC_RMID, NULL) == -1)
    errExit("msgctl failed");
}
