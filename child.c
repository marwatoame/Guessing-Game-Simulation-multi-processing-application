#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/signal.h>
#include "error_codes.h"

#define LOWER_LIMIT   1
#define UPPER_LIMIT   100

char * child_file;
int child_sig;
int guess;

void new_round(int);
void write_to_file();

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Invalid arguments: %d\n", argc);
    exit(-1);
  }

  // get the file path
  child_file = argv[1];
  child_sig = atoi(argv[2]);

  // set the seed with the PID
  srand(time(NULL) % getpid());

  // set the handler
  if (signal(SIGUSR1, new_round) == SIG_ERR) {
    perror("handle error");
    kill(getppid(), SIGTERM);
  }

  // start the first guess
  new_round(SIGUSR1);

  // wait for the remaining rounds
  while (1) {
    if (guess != -1)
      write_to_file();
  }
  
  return 0;
}

void new_round(int signum){
  // set the guessing flag
  guess = 0;
}

void write_to_file() {
  FILE * fileptr;
  fileptr = fopen(child_file, "w");
  if (fileptr == NULL){
    perror("fopen error");
    kill(getppid(), SIGTERM);
  }

  // generate the guess
  guess = rand() % (UPPER_LIMIT - LOWER_LIMIT + 1) + LOWER_LIMIT;

  // write the guess
  fprintf(fileptr, "%d", guess);

  if (fclose(fileptr)) {
    perror("fclose error");
    kill(getppid(), SIGTERM);
  }

  // reset the guessing flag
  guess = -1;
  
  // inform the parent
  kill(getppid(), child_sig);
}