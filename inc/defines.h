/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#define PATH_BUFFER_SIZE 150
#define MAX_IPC_FILE 50

#define PATH_FIFO1 "/tmp/fifo1"
#define PATH_FIFO2 "/tmp/fifo2"
#define PATH_SHARED_MEMORY "/tmp"
#define PATH_MESSAGE_QUEUE "/tmp"
#define PATH_SEMAPHORE "/tmp"

#define KEY_SHARED_MEMORY 'a'
#define KEY_SHARED_MEMORY_FILES 'b'
#define KEY_MESSAGE_QUEUE 'm'
#define KEY_SEMAPHORE 's'
#define KEY_FILE_SEMAPHORE 'f'
#define KEY_RECEIVERS 'r'

#define SEM_REQUEST 0
#define SEM_DATA_READY 1

#define FILE_MAX_SIZE 4096
#define FILE_BUFFER_SIZE 1025

#define STRING_TO_SEARCH "sendme_"
#define STRING_FILE_OUT "_out"
#define STRING_IN_FILE 68 + PATH_BUFFER_SIZE

#define STRING_COMPLETE_STATUS "RECIVED"
#define STRING_OUTPUT "[Parte %d, del file %s, spedita dal processo %d tramite %s]\n"


struct Request {
  int pid;
  char pathname[PATH_BUFFER_SIZE];
  char content[FILE_BUFFER_SIZE];
};

struct SharedMemoryRequest {
  struct Request requests[MAX_IPC_FILE];
  int index[MAX_IPC_FILE];
};

struct FileSet {
  int pid;
  char pathname[PATH_BUFFER_SIZE];
  char contentFF1[FILE_BUFFER_SIZE];
  char contentFF2[FILE_BUFFER_SIZE];
  char contentMQ[FILE_BUFFER_SIZE];
  char contentSM[FILE_BUFFER_SIZE];
};
