/// @file server.c
/// @brief Contiene l'implementazione del server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <unistd.h>

#include "inc/defines.h"
#include "inc/err_exit.h"
#include "inc/fifo.h"
#include "inc/semaphore.h"
#include "inc/shared_memory.h"
#include "inc/message_queue.h"
#include "inc/keys_utils.h"
#include "inc/file_utils.h"
#include "inc/string_utils.h"


void sigHandler(int sig);
void addFile(struct FileSet set[], int length, struct Request *file, int index);
void writeOnFile(int fd, char *content);
void formatStringOutPut(char *buffer, int part, char *path,int pid);

// Variables
int semSMId;
int semSMMutexId;
int semFFId;
int shmId = -1;
int shmFileId = -1;
int serverFIFO1 = -1;
int serverFIFO1_extra = -1;
int serverFIFO2 = -1;
int serverFIFO2_extra = -1;
int msqId = -1;
struct Request *reqSM = NULL;
struct SharedMemoryRequest *reqFileSM = NULL;


int main(int argc, char * argv[]) {
  // handle segnale SIGINT
  if (signal(SIGINT, sigHandler) == SIG_ERR)
    errExit("change signal handler failed");

  // create FIFOs
  printf("<Server> creating FIFO %s...\n", PATH_FIFO1);
  makeFIFO(PATH_FIFO1);
  printf("<Server> creating FIFO %s...\n", PATH_FIFO2);
  makeFIFO(PATH_FIFO2);

  // create MessageQueue
  key_t msgKey = get_key(PATH_MESSAGE_QUEUE, KEY_MESSAGE_QUEUE);
  printf("<Server> creating Message Queue...\n");
  msqId = create_message_queue(msgKey);

  // create SharedMemory
  key_t shmKey = get_key(PATH_SHARED_MEMORY, KEY_SHARED_MEMORY);
  printf("<Server> allocating a shared memory segment...\n");
  shmId = alloc_shared_memory(shmKey, sizeof(struct Request));

  // get Key for SharedMemory for Files
  key_t shmFileKey = get_key(PATH_SHARED_MEMORY, KEY_SHARED_MEMORY_FILES);
  printf("<Server> allocating a shared memory segment for files...\n");
  shmFileId = alloc_shared_memory(shmFileKey, sizeof(struct SharedMemoryRequest));

  // create Semaphore
  key_t semKeySM = get_key(PATH_SHARED_MEMORY, KEY_SEMAPHORE);
  printf("<Server> creating a semaphore set for SM...\n");
  unsigned short values[] = {1, 0};
  semSMId = create_sem_set(semKeySM, 2, values);

  key_t semKeyMutexSM = get_key(PATH_SHARED_MEMORY, KEY_FILE_SEMAPHORE);
  semSMMutexId = create_sem(semKeyMutexSM, 1);

  // create Semaphore for Fifo
  key_t semKeyFF = get_key(PATH_FIFO1, KEY_SEMAPHORE);
  printf("<Server> creating a semaphore set for FIFO...\n");
  semFFId = create_sem(semKeyFF, (unsigned short)0);

  // open FIFOs
  printf("<Server> waiting for a client in FIFO %s...\n", PATH_FIFO1);
  // serverFIFO1 = openFIFO(PATH_FIFO1, O_RDONLY); // blocking
  serverFIFO1 = openFIFO(PATH_FIFO1, O_RDONLY | O_NONBLOCK); // non blocking
  serverFIFO1_extra = openFIFO(PATH_FIFO1, O_WRONLY); // prevent EOF

  printf("<Server> waiting for a client in FIFO %s...\n", PATH_FIFO2);
  // serverFIFO2 = openFIFO(PATH_FIFO2, O_RDONLY); // blocking
  serverFIFO2 = openFIFO(PATH_FIFO2, O_RDONLY | O_NONBLOCK); // non blocking
  serverFIFO2_extra = openFIFO(PATH_FIFO2, O_WRONLY); // prevent EOF

  // open SharedMemory
  printf("<Server> attaching the shared memory...\n");
  reqSM = (struct Request*)get_shared_memory(shmId, 0);

  printf("<Server> attaching the shared memory for files...\n");
  reqFileSM = (struct SharedMemoryRequest*)get_shared_memory(shmFileId, 0);

  while(1) {
    // Get number of file from client
    printf("<Server> waiting for number of files...\n");
    semOp(semFFId, 0, -1);
    printf("<Server> waiting number of files passed\n");

    int files;
    int bR = read(serverFIFO1, &files, sizeof(int));
    if (bR == -1)
      errExit("<Server> FIFO is broken");
    else if (bR == 0) {
      printf("<Server> No more client in FIFO\n");
      break;
    } else if (bR != sizeof(int)) {
      printf("<Server> not receive a number\n");
      continue;
    } else
      printf("<Server> preparing for %d files...\n", files);

    // Conferma al client
    sprintf(reqSM->content, "%d", files);
    semOp(semSMId, SEM_DATA_READY, 1);

    struct FileSet set[files];
    for(int i = 0; i < files; i++) // reset set
      set[i].pid = 0;

    // get receivers semaphore
    key_t semKeyREC= get_key(PATH_SEMAPHORE, KEY_RECEIVERS);
    printf("<Server> get semaphore set for receivers...\n");
    int semRECId = semGet(semKeyREC, 4);
    printf("<Server> found semaphore with ID: %i\n", semRECId);

    // si mette in ascolto ciclicamente sui 4 canali
    for(int n = 0; n < files * 4; ) {
      // FIFO1
      // printf("<Server> reading on FIFO1...\n");
      struct Request reqFIFO1;
      bR = read(serverFIFO1, &reqFIFO1, sizeof(struct Request));
      if (bR == -1) {
        if (errno != EAGAIN)
          errExit("<Server> FIFO1 is broken");
      } else if (bR == sizeof(struct Request)){
        semOp(semRECId, 0, 1);
        printf("<Server> received data from %i on FF1\n", reqFIFO1.pid);

        addFile(set, files, &reqFIFO1, 0);
        n++;
      }

      // FIFO2
      // printf("<Server> reading on FIFO2...\n");
      struct Request reqFIFO2;
      bR = read(serverFIFO2, &reqFIFO2, sizeof(struct Request));
      if (bR == -1) {
        if (errno != EAGAIN)
          errExit("<Server> FIFO2 is broken");
      } else if (bR == sizeof(struct Request)) {
        semOp(semRECId, 1, 1);
        printf("<Server> received data from %i on FF2\n", reqFIFO2.pid);

        addFile(set, files, &reqFIFO2, 1);
        n++;
      }

      // MsgQueue
      // printf("<Server> reading on MQ...\n");
      struct Request reqMQ;
      size_t mSize = sizeof(struct Request) - sizeof(long);
      // if (msgrcv(msqId, &reqMQ, mSize, 0, 0) == -1) // blocking
      if (msgrcv(msqId, &reqMQ, mSize, 0, IPC_NOWAIT) == -1) {
        if (errno != ENOMSG)
          errExit("<Server> MQ is broken");
      } else {
        semOp(semRECId, 2, 1);
        printf("<Server> received data from %i on MQ\n", reqMQ.pid);

        addFile(set, files, &reqMQ, 2);
        n++;
      }

      // SharedMemory
      if (semOpNoWait(semSMMutexId, 0, -1) == -1) {
        if (errno != EAGAIN)
          errExit("semop failed asd");
      } else {
        // printf("<Server> reading on SharedMemory...\n");
        for (int i=0; i < MAX_IPC_FILE; i++) {
          if (reqFileSM->index[i] == 1) {
            printf("<Server> received data from %i on SM\n", reqFileSM->requests[i].pid);
            addFile(set, files, &reqFileSM->requests[i], 3);

            reqFileSM->index[i] = 0;
            n++;
            semOp(semRECId, 3, 1);
            break; // optional
          }
        }
        semOp(semSMMutexId, 0, 1);
      }
      // printf("<Server> reading on SharedMemory...\n");
      // if (semOpNoWait(semSMId, SEM_DATA_READY, -1) == -1) {
      //   if (errno != EAGAIN)
      //     errExit("semop failed");
      // } else {
      //   // semOp(semSMId, SEM_DATA_READY, -1);
      //   printf("<Server> received data from %i on SM\n", reqSM->pid);
      //   addFile(set, files, reqSM, 3);
      //   n++;
      //   semOp(semSMId, SEM_REQUEST, 1);
      // }
    }
    printf("<Server> Files received\n");

    for(int i=0; i < files; i++) {
      // printf("<Server> File %i -> FF1: %s - FF2: %s - MQ: %s - SM: %s\n", i+1, set[i].contentFF1, set[i].contentFF2, set[i].contentMQ, set[i].contentSM);
      char pathname[PATH_BUFFER_SIZE] = "";
      const char *ext = getFilenameExt(set[i].pathname);
      strcat(pathname, set[i].pathname);
      strRemoveSuffix(pathname, ext);
      strcat(pathname, STRING_FILE_OUT);
      strcat(pathname, ext);

      int fileD = open(pathname, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
      if (fileD == -1)
        errExit("open failed");
      if (lseek(fileD, 0, SEEK_END) == -1)
        errExit("lssek failed");

      char outBuffer[STRING_IN_FILE] = "";

      formatStringOutPut(outBuffer, 1, set[i].pathname, set[i].pid);
      writeOnFile(fileD, outBuffer);
      writeOnFile(fileD, set[i].contentFF1);
      writeOnFile(fileD, "\n\n");

      formatStringOutPut(outBuffer, 2, set[i].pathname, set[i].pid);
      writeOnFile(fileD, outBuffer);
      writeOnFile(fileD, set[i].contentFF2);
      writeOnFile(fileD, "\n\n");  

      formatStringOutPut(outBuffer, 3, set[i].pathname, set[i].pid);
      writeOnFile(fileD, outBuffer);
      writeOnFile(fileD, set[i].contentMQ);
      writeOnFile(fileD, "\n\n");

      formatStringOutPut(outBuffer, 4, set[i].pathname, set[i].pid);
      writeOnFile(fileD, outBuffer);
      writeOnFile(fileD, set[i].contentSM);
      writeOnFile(fileD, "\n");

      close(fileD);
    }
    printf("<Server> Files saved\n");

    // Conferma al client su MQ
    printf("<Server> Conferma al Client su MQ\n");
    if (msgsnd(msqId, STRING_COMPLETE_STATUS, sizeof(STRING_COMPLETE_STATUS) - sizeof(long), 0) == -1)
      errExit("msgsnd failed");
  }

  return 0;
}

void sigHandler(int sig) {
  // close FIFO
  printf("<Server> closing FIFOs...\n");
  closeFIFO(serverFIFO1_extra);
  closeFIFO(serverFIFO1);
  closeFIFO(serverFIFO2_extra);
  closeFIFO(serverFIFO2);

  // close SharedMemory
  printf("<Server> detaching the file shared memory...\n");
  free_shared_memory(reqFileSM);
  printf("<Server> detaching the shared memory...\n");
  free_shared_memory(reqSM);

  // remove FIFO
  printf("<Server> unlinking FIFOs...\n");
  unlinkFIFO(PATH_FIFO1);
  unlinkFIFO(PATH_FIFO2);

  // remove Message Queue
  printf("<Server> removing message queue...\n");
  remove_message_queue(msqId);

  // remove Semaphore
  printf("<Server> removing semaphore...\n");
  remove_semaphore(semSMId);

  // remove Semaphore Mutex
  printf("<Server> removing mutex...\n");
  remove_semaphore(semSMMutexId);
  
  // remove SharedMemory
  printf("<Server> removing the shared memory...\n");
  remove_shared_memory(shmId);

   // remove SharedMemory
  printf("<Server> removing the file shared memory...\n");
  remove_shared_memory(shmFileId);

  printf("<Server> Closed\n");
  exit(0);
}

void addFile(struct FileSet set[], int length, struct Request *file, int index) {
  for(int i=0; i < length; i++) {
    if(set[i].pid == 0 || set[i].pid == file->pid) {
      if (set[i].pid == 0) {
        set[i].pid = file->pid;
        strcpy(set[i].pathname, file->pathname);
      }
      if (index == 0)
        strcpy(set[i].contentFF1, file->content);
      else if (index == 1)
        strcpy(set[i].contentFF2, file->content);
      else if (index == 2)
        strcpy(set[i].contentMQ, file->content);
      else if (index == 3)
        strcpy(set[i].contentSM, file->content);
      break;
    }
  }
}

void writeOnFile(int fd, char *content) {
  int length = strlen(content);
  if (write(fd, content, length) != length)
    errExit("write failed");
}

void formatStringOutPut(char *buffer, int part, char *path,int pid){
  switch(part) {
    case 1:
      sprintf(buffer, STRING_OUTPUT, part, path, pid, "FIFO1");
      break;
    case 2:
      sprintf(buffer, STRING_OUTPUT, part, path, pid, "FIFO2");
      break;
    case 3:
      sprintf(buffer,STRING_OUTPUT, part, path, pid, "MsgQueue");
      break;
    case 4:
      sprintf(buffer, STRING_OUTPUT, part, path, pid, "ShdMem");
      break;
  }
}
