#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

pid_t *child_pids = NULL;
int timeout = 0;
volatile sig_atomic_t timeout_reached = 0;

void timeout_handler(int sig) {
    if (sig == SIGALRM) {
        timeout_reached = 1;
        printf("Timeout reached!\n");
        for (int i = 0; child_pids[i] != 0; i++) {
            if (child_pids[i] > 0) {
                kill(child_pids[i], SIGKILL);
            }
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  timeout = 0;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4:
            timeout = atoi(optarg);
            if (timeout <= 0) {
                printf("timeout must be a positive number\n");
                return 1;
            }
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  child_pids = malloc(sizeof(pid_t) * (pnum + 1));
  for (int i = 0; i <= pnum; i++) {
      child_pids[i] = 0;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  int pipes[pnum][2];
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        printf("Pipe creation failed!\n");
        return 1;
      }
    }
  }

  bool *process_completed = malloc(sizeof(bool) * pnum);
  for (int i = 0; i < pnum; i++) {
      process_completed[i] = false;
  }

  if (timeout > 0) {
      signal(SIGALRM, timeout_handler);
      alarm(timeout);
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      child_pids[i] = child_pid;
      if (child_pid == 0) {
        int segment_size = array_size / pnum;
        int begin = i * segment_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * segment_size;

        struct MinMax local_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[32];
          sprintf(filename, "min_max_%d.txt", i);
          FILE *file = fopen(filename, "w");
          if (file != NULL) {
            fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
            fclose(file);
          }
        } else {
          close(pipes[i][0]);
          write(pipes[i][1], &local_min_max.min, sizeof(int));
          write(pipes[i][1], &local_min_max.max, sizeof(int));
          close(pipes[i][1]);
        }
        free(array);
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  while (active_child_processes > 0) {
      int status;
      pid_t finished_pid = waitpid(-1, &status, WNOHANG);
      
      if (finished_pid > 0) {
          active_child_processes -= 1;
          
          for (int i = 0; i < pnum; i++) {
              if (child_pids[i] == finished_pid) {
                  process_completed[i] = true;
                  child_pids[i] = 0;
                  break;
              }
          }
          
          if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
          } else {
              printf("Child process %d terminated abnormally\n", finished_pid);
          }
      } else if (finished_pid == 0) {
          if (timeout_reached) {
              break;
          }
          usleep(10000);
      } else {
          if (errno != ECHILD) {
              perror("waitpid");
          }
          break;
      }
  }

  if (timeout > 0) {
      alarm(0);
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  int completed_count = 0;
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    bool valid_data = false;

    if (process_completed[i]) {
        if (with_files) {
          char filename[32];
          sprintf(filename, "min_max_%d.txt", i);
          FILE *file = fopen(filename, "r");
          if (file != NULL) {
            if (fscanf(file, "%d %d", &min, &max) == 2) {
                valid_data = true;
                completed_count++;
            }
            fclose(file);
            remove(filename);
          }
        } else {
          close(pipes[i][1]);
          if (read(pipes[i][0], &min, sizeof(int)) == sizeof(int) &&
              read(pipes[i][0], &max, sizeof(int)) == sizeof(int)) {
              valid_data = true;
              completed_count++;
          }
          close(pipes[i][0]);
        }

        if (valid_data) {
            if (min < min_max.min) min_max.min = min;
            if (max > min_max.max) min_max.max = max;
        }
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);
  free(process_completed);

  if (completed_count > 0) {
      printf("Min: %d\n", min_max.min);
      printf("Max: %d\n", min_max.max);
      printf("Completed processes: %d/%d\n", completed_count, pnum);
  } else {
      printf("No processes completed successfully within timeout\n");
      min_max.min = 0;
      min_max.max = 0;
  }
  
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}