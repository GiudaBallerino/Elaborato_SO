/// @file client.c
/// @brief Contiene l'implementazione del client

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>

#include "inc/defines.h"
#include "inc/err_exit.h"
#include "inc/fifo.h"
#include "inc/semaphore.h"
#include "inc/shared_memory.h"
#include "inc/message_queue.h"
#include "inc/string_utils.h"
#include "inc/file_utils.h"
#include "inc/keys_utils.h"


void sigHandler(int sig);
int search(char directory[]);

// Variables
struct list *list;

int serverFIFO1 = -1;
int serverFIFO2 = -1;
int msqId = -1;
struct Request *reqSM = NULL;
struct SharedMemoryRequest *reqFileSM = NULL;
int semId;
int semRECId;


int main(int argc, char * argv[]) {
  // if (argc > 2) {
  if (argc != 2) {
    printf("Usage: %s <sourceDir>\n", argv[0]);
    return 1;
  }

  list = (struct list*) malloc(sizeof(struct list));
  list->entry = NULL;

  char sourceDir[PATH_BUFFER_SIZE]; // = "/home/runner/Elaborato/myDir";
  if (argc == 2)
    strcpy(sourceDir, argv[1]);

  // Open FIFO
  printf("<Client> opening FIFO1 %s...\n", PATH_FIFO1);
  serverFIFO1 = openFIFO(PATH_FIFO1, O_WRONLY);
  printf("<Client> opening FIFO2 %s...\n", PATH_FIFO2);
  serverFIFO2 = openFIFO(PATH_FIFO2, O_WRONLY);

  // Open SharedMemory
  key_t shmKeyServer = get_key(PATH_SHARED_MEMORY, KEY_SHARED_MEMORY);
  printf("<Client> getting the server's shared memory...\n");
  int shmIdServer = alloc_shared_memory(shmKeyServer, sizeof(struct Request));
  printf("<Client> attaching the server's shared memory...\n");
  reqSM = (struct Request *)get_shared_memory(shmIdServer, 0);
  
  // Open File SharedMemory
  key_t shmFileKey = get_key(PATH_SHARED_MEMORY, KEY_SHARED_MEMORY_FILES);
  printf("<Client> getting the server's shared memory for files...\n");
  int shmFileIdServer = alloc_shared_memory(shmFileKey, sizeof(struct Request));
  printf("<Client> attaching the server's shared memory for files...\n");
  reqFileSM = (struct SharedMemoryRequest*)get_shared_memory(shmFileIdServer, 0);

  // Open MsgQueue
  key_t msgKey = get_key(PATH_MESSAGE_QUEUE, KEY_MESSAGE_QUEUE);
  printf("<Server> creating Message Queue...\n");
  msqId = open_message_queue(msgKey);

  // create receivers semaphore
  key_t semKeyREC = get_key(PATH_SEMAPHORE, KEY_RECEIVERS);
  printf("<Client> get semaphore set for receivers...\n");
  unsigned short values[] = {MAX_IPC_FILE, MAX_IPC_FILE, MAX_IPC_FILE, MAX_IPC_FILE};
  semRECId = create_sem_set(semKeyREC, 4, values);

  // get Semaphore FIFO
  key_t semFFKey = get_key(PATH_FIFO1, KEY_SEMAPHORE);
  printf("<Client> get semaphore set for FIFO...\n");
  int semFFId = semGet(semFFKey, 1);

  // get Semaphore SM
  key_t semKey = get_key(PATH_SHARED_MEMORY, KEY_SEMAPHORE);
  printf("<Client> get semaphore set for SM...\n");
  int semSMId = semGet(semKey, 2);

  // get mutex Semaphore SM
  key_t semKeyMutexSM = get_key(PATH_SHARED_MEMORY, KEY_FILE_SEMAPHORE);
  printf("<Client> get mutex semaphore set for SM...\n");
  int semSMMutexId = semGet(semKeyMutexSM, 1);

  // create Semaphore for Fork
  printf("<Client> creating a semaphore set for Fork...\n");
  semId = create_sem(IPC_PRIVATE, 0 /*ignored*/);

  // maschera che ammette solo i segnali SIGINT e SIGUSR1
  sigset_t signalsSet;
  sigfillset(&signalsSet);
  sigdelset(&signalsSet, SIGINT);
  sigdelset(&signalsSet, SIGUSR1);

  // maschera che blocca tutti i segnali
  sigset_t signalsSetAll;
  sigfillset(&signalsSetAll);

  if (signal(SIGINT, sigHandler) == SIG_ERR)
    errExit("change signal handler failed");
  if (signal(SIGUSR1, sigHandler) == SIG_ERR)
    errExit("change signal handler failed");

  while(1) {
    sigprocmask(SIG_SETMASK, &signalsSet, NULL); // set mask
    pause(); // attesa segnale SIGINT

    // blocca tutti i segnali (compresi SIGUSR1 e SIGINT)
    sigprocmask(SIG_SETMASK, &signalsSetAll, NULL);

    // imposta la sua directory corrente
    if (chdir(sourceDir) == -1)
      errExit("chdir failed");

    char *username = getenv("USER");
    if (username == NULL)
      username = "Unknown";

    char currDir[PATH_BUFFER_SIZE];
    if (getcwd(currDir, PATH_BUFFER_SIZE) == NULL)
      errExit("getcwd failed");

    printf("Ciao %s, ora inizio l’invio dei file contenuti in %s\n", username, currDir); // TODO: change to system call

    // cerca tutti i file con "sendme_" e size inferiore a 4KByte
    reset_list(list);
    int files = search(currDir);
    printf("<Client> %i regular file found\n", files);

    // invia tramite FIFO1 al server il numero di file
    if (write(serverFIFO1, &files, sizeof(int)) != sizeof(int))
      errExit("write FIFO1 failed");
    semOp(semFFId, 0, 1);

    // attesa conferma dal server in SharedMemory
    semOp(semSMId, SEM_DATA_READY, -1);
    int response = atoi(reqSM->content);
    if (response != files)
      errExit("message corrupted");

    // set Semaphore
    union semun arg;
    arg.val = files;
    if (semctl(semId, 0 /*ignored*/, SETVAL, arg) == -1)
      errExit("semctl SETALL failed");

    // per ogni file genera un processo figlio Client_i
    int index = 1;
    pid_t pid;

    struct file *f = list->entry;
    while (f != NULL) {
      pid = fork();
      if (pid < 0)
        printf("<Client> Child %i not created!\n", index);
      else if (pid == 0) {
        // operazioni dei <Client_i> ...
        pid = getpid();
        printf("<Client_%i> PID: %i, File %s\n", index, pid, f->pathname);

        int fd = openFile(f->pathname);
        int size = getFileSizeFromFD(fd);
        int indexSender = 0;
        int part = 4;

        char buffer[4][FILE_BUFFER_SIZE];
        ssize_t bR = 0;
        do {
          int size_part = (size + part - 1) / part;
          bR = read(fd, buffer[indexSender], size_part);
          if (bR > 0) {
            buffer[indexSender][bR]= '\0';
            // printf("<Client_%i> %s\n", index, buffer[indexSender]);

            size -= size_part;
            part--;
            indexSender++;
          }
        } while (bR > 0 && part > 0);

        printf("<Client_%i> Wait for all child\n", index);
        semOp(semId, 0, -1);
        semOp(semId, 0, 0); // wait semaphore to be zero

        struct Request fifo1Part;
        fifo1Part.pid = pid;
        strcpy(fifo1Part.content, buffer[0]);
        strcpy(fifo1Part.pathname, f->pathname);

        struct Request fifo2Part;
        fifo2Part.pid = pid;
        strcpy(fifo2Part.content, buffer[1]);
        strcpy(fifo2Part.pathname, f->pathname);

        struct Request msqPart;
        msqPart.mtype = 1;
        msqPart.pid = pid;
        strcpy(msqPart.pathname, f->pathname);
        strcpy(msqPart.content, buffer[2]);

        int sended[] = {1, 1, 1, 1};

        while (sended[0] == 1
          || sended[1] == 1
          || sended[2] == 1
          || sended[3] == 1) {
          // FIFO1
          if (sended[0] == 1) {
            if (semOpNoWait(semRECId, 0, -1) == -1) {
              if (errno != EAGAIN)
                errExit("semop failed");
            } else {
              // printf("<Client_%i> send %s on FIFO1\n", index, buffer[0]);
              if (write(serverFIFO1, &fifo1Part, sizeof(struct Request)) != sizeof(struct Request))
                errExit("write FIFO1 failed");
              else
                sended[0] = 0;
            }
          }
          // semOp(semRECId, 0, -1);
          // if (write(serverFIFO1, &fifo1Part, sizeof(struct Request)) != sizeof(struct Request))
          //   errExit("write FIFO1 failed");
  
          // FIFO2
          if (sended[1] == 1) {
            if (semOpNoWait(semRECId, 1, -1) == -1) {
              if (errno != EAGAIN)
                errExit("semop failed");
            } else {
              // printf("<Client_%i> sending %s on FIFO2\n", index, buffer[1]);
              if (write(serverFIFO2, &fifo2Part, sizeof(struct Request)) != sizeof(struct Request))
                errExit("write FIFO2 failed");
              else
                sended[1] = 0;
            }
          }
          // semOp(semRECId, 1, -1);
          // if (write(serverFIFO2, &fifo2Part, sizeof(struct Request)) != sizeof(struct Request))
          //   errExit("write FIFO2 failed");
  
          // MsgQueue
          if (sended[2] == 1) {
            if (semOpNoWait(semRECId, 2, -1) == -1) {
              if (errno != EAGAIN)
                errExit("semop failed");
            } else {
              // printf("<Client_%i> sending %s on MQ\n", index, buffer[2]);
              size_t mSize = sizeof(struct Request) - sizeof(long);
              if (msgsnd(msqId, &msqPart, mSize, 0) == -1)
                errExit("msgsnd failed");
              else
                sended[2] = 0;
            }
          }
          // semOp(semRECId, 2, -1);
          // size_t mSize = sizeof(struct Request) - sizeof(long);
          // if (msgsnd(msqId, &msqPart, mSize, 0) == -1)
          //   errExit("msgsnd failed");
  
          // SharedMemory
          if (sended[3] == 1) {
            if (semOpNoWait(semRECId, 3, -1) == -1) {
              if (errno != EAGAIN)
                errExit("semop failed");
            } else {
              if (semOpNoWait(semSMMutexId, 0, -1) == -1) {
                if (errno != EAGAIN)
                  errExit("semop failed");
                else
                  semOp(semRECId, 3, 1); // libera semaforo
              } else {
                int index = 0;
                for(; index < MAX_IPC_FILE; index++)
                  if (reqFileSM->index[index] == 0)
                    break;
                reqFileSM->requests[index].pid = pid;
                strcpy(reqFileSM->requests[index].pathname, f->pathname);
                strcpy(reqFileSM->requests[index].content, buffer[3]);
  
                reqFileSM->index[index] = 1;
                sended[3] = 0;
                semOp(semSMMutexId, 0, 1);
              }
            }
          }
          
          // semOp(semSMId, SEM_REQUEST, -1);
          // // printf("<Client_%i> sending %s on SM\n", index, buffer[3]);
          // reqSM->pid = pid;
          // strcpy(reqSM->pathname, f->pathname);
          // strcpy(reqSM->content, buffer[3]);
          // semOp(semSMId, SEM_DATA_READY, 1);
        }

        close(fd); // close file
        printf("<Client_%i> Ended\n", index);

        exit(0); // IMPORTANT
      }
      f = f->next;
      index++;
    }

    while(wait(NULL) != -1);
    printf("<Client> All child ended\n");
    // int status = 0;
    // while ((pid = wait(&status)) != -1);
    //   printf("Child %d exit status = %d\n", pid, WEXITSTATUS(status));

    // attesa conferma dal server in MQ
    semOp(semSMId, SEM_DATA_READY, -1);
    char confirm[10];
    if (msgrcv(msqId, &confirm, sizeof(confirm)-sizeof(long), 0, 0) == -1)
      errExit("<Client> MQ is broken");

    if(strcmp(confirm, STRING_COMPLETE_STATUS) == 0)
      printf("<Client> Server has recived and saved data\n");
    else
      printf("<Client> Server has generated an error\n");
  }

  return 0;
}

int search(char path[]) {
  int count = 0;

  // printf("Scanning dir: %s\n", buffer);
  DIR *dp = opendir(path);
  if (dp == NULL)
    errExit("opendir failed");

  struct dirent *dentry;
  while ((dentry = readdir(dp)) != NULL) {
    if (strcmp(dentry->d_name, ".") == 0
      || strcmp(dentry->d_name, "..") == 0)
      continue;

    if (dentry->d_type == DT_REG) {
      size_t length = appendToPath(path, dentry->d_name);

      if (strStartWith(dentry->d_name, STRING_TO_SEARCH) == 0
        // && strEndsWith(dentry->d_name, STRING_FILE_OUT) == 0
        && (getFileSize(path) > FILE_MAX_SIZE) == 0) {
        // printf("Regular file: %s\n", path);
        append_file(path, list);
        count++;
      }
      path[length] = '\0'; // reset current path
    }
    else if (dentry->d_type == DT_DIR) { // recursive function
      size_t length = appendToPath(path, dentry->d_name);
      count += search(path);
      path[length] = '\0'; // reset current path
    }
  }
  closedir(dp);
  return count;
}

void sigHandler(int sig) {
  if (sig == SIGUSR1) {
    // close FIFO
    printf("<Client> closing FIFOs...\n");
    closeFIFO(serverFIFO1);
    closeFIFO(serverFIFO2);

    // close SharedMemory
    printf("<Client> detaching from server's shared memory...\n");
    free_shared_memory(reqSM);

    // close SharedMemory
    printf("<Client> detaching from server's shared memory...\n");
    free_shared_memory(reqFileSM);

    // remove Semaphore
    printf("<Client> removing semaphore...\n");
    remove_semaphore(semId);
    printf("<Client> removing semaphore...\n");
    remove_semaphore(semRECId);

    printf("<Client> Closed\n");
    exit(0);
  }
}
