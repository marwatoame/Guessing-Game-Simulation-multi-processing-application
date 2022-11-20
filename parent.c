#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/signal.h>
#include <sys/wait.h>
#include "error_codes.h"

#define NUM_OF_PLAYERS     2

void terminate(int, int);
void terminate_all(int);
void players_ready();
void response_received(int);
void collect_responses();
void calculate_points();
void announce_new_round();
void end_game();

pid_t child_pid       [NUM_OF_PLAYERS]; // PID for each child process
char * child_file     [NUM_OF_PLAYERS]; // the path of the file for each player
int guess             [NUM_OF_PLAYERS], // the guess value of each player
    points            [NUM_OF_PLAYERS], // points for each player
    received_responses,                 // number of received responses
    created_children;                   // number of children created so far

int main() {

  // initialize
  int child_sig[NUM_OF_PLAYERS] = {SIGINT, SIGQUIT};
  received_responses = 0, created_children = 0;
  for (int i = 0; i < NUM_OF_PLAYERS; i++) {
    child_file[i] = (char *) malloc(20 * sizeof(char));
    sprintf(child_file[i], "./P%d.txt", i + 1);
    points[i] = 0;
  }

  // Handling signals
  for (int i = 0; i < NUM_OF_PLAYERS; i++) {
    if (signal(child_sig[i], response_received) == SIG_ERR) {
      perror("handle error");
      exit(HANDLER_SET_ERR);
    }
  }
  if (signal(SIGTERM, terminate_all) == SIG_ERR) {
    perror("handle error");
    exit(HANDLER_SET_ERR);
  }

  // Create players (children)
  for (int i = 0; i < NUM_OF_PLAYERS; i++) {
    pid_t pid = fork();

    if (pid == 0) {
      // each child will execute the child.c with the corresponding file
      char sig_num[4];
      sprintf(sig_num, "%d", child_sig[i]);
      execl("./child", "./child", child_file[i], sig_num, (char *) NULL);
    } else if (pid == -1) {
      perror("fork error");
      terminate(created_children, FORK_ERR);
    }

    // register the child
    child_pid[i] = pid;
    created_children++;
  }

  // wait for guesses
  while (1) {
    if (received_responses == NUM_OF_PLAYERS)
      players_ready();
  }
  
  return 0;
}

void terminate_all(int signum) {
  // if SIGTERM received we have to terminate the children and the parent
  terminate(created_children, TERMINATED);
}

void response_received(int signum) {
  // one more response recieved
  received_responses++;
}

void players_ready() {
  collect_responses();
  calculate_points();

  // end the game if one player at least reached 10 points
  for (int i = 0; i < NUM_OF_PLAYERS; i++)
    if (points[i] == 10)
      end_game();
  
  announce_new_round();
}

void collect_responses() {
  printf("Guesses: \n");

  // read all files
  for (int i = 0; i < NUM_OF_PLAYERS; i++) {
    // open the file
    FILE * fileptr;
    fileptr = fopen(child_file[i], "r");
    if (fileptr == NULL) {
      perror("fopen error");
      terminate(NUM_OF_PLAYERS, FILE_OPEN_ERR);
    }

    // read the guess
    fscanf(fileptr, "%d", &guess[i]);

    // print it to the screen
    printf("%d\t", guess[i]);

    // close the file
    if (fclose(fileptr)) {
      perror("fcolse error");
      terminate(NUM_OF_PLAYERS, FILE_CLOSE_ERR);
    }

    // remove the file
    if (remove(child_file[i])) {
      perror("remvoe error");
      terminate(NUM_OF_PLAYERS, FILE_REMOVE_ERR);
    }
  }
  printf("\n");
}

void calculate_points() {
  // find the max guess
  int max_guess = 0;
  for (int i = 0; i < NUM_OF_PLAYERS; i++)
    if (max_guess < guess[i])
      max_guess = guess[i];

  printf("Player(s) with max guess: \n");
  // any player has the max guess will get 1 point
  for (int i = 0; i < NUM_OF_PLAYERS; i++)
    if (guess[i] == max_guess) {
      printf("P%d\t", i + 1);
      points[i]++;
    }
  
  printf("\n\n");
}

void announce_new_round() {
  // reset the reponse counter
  received_responses = 0;

  // send SIGUSR1 to all players
  for (int i = 0; i < NUM_OF_PLAYERS; i++) {
    kill(child_pid[i], SIGUSR1);
  }
}

void end_game() {
  printf("Winner(s): \n");
  
  for (int i = 0; i < NUM_OF_PLAYERS; i++) {
    // if the player got 10 points declare him as a winner
    if (points[i] == 10) {
      printf("P%d\t", i + 1);
    }
  }

  printf("\n");

  // terminate with success
  terminate(NUM_OF_PLAYERS, EXIT_SUCCESS);
}

void terminate(int created_processes, int exit_code) {
  // Kill the childs and exit
  for (int i = 0; i < created_processes; i++) {
    kill(child_pid[i], SIGTERM);
  }

  exit(exit_code);
}