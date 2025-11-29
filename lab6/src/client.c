#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "pthread.h"
#include "common.h"

struct ThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};

int ReadServersFromFile(const char *filename, struct Server **servers) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Failed to open servers file");
    return -1;
  }

  int capacity = 10;
  int count = 0;
  *servers = malloc(sizeof(struct Server) * capacity);

  char line[255];
  while (fgets(line, sizeof(line), file)) {
    line[strcspn(line, "\n")] = '\0';
    
    char *colon = strchr(line, ':');
    if (!colon) {
      fprintf(stderr, "Invalid server format: %s\n", line);
      continue;
    }
    
    *colon = '\0';
    int port = atoi(colon + 1);
    
    if (count >= capacity) {
      capacity *= 2;
      *servers = realloc(*servers, sizeof(struct Server) * capacity);
    }
    
    strncpy((*servers)[count].ip, line, sizeof((*servers)[count].ip) - 1);
    (*servers)[count].ip[sizeof((*servers)[count].ip) - 1] = '\0';
    (*servers)[count].port = port;
    count++;
  }

  fclose(file);
  return count;
}

void *ServerThread(void *args) {
  struct ThreadArgs *thread_args = (struct ThreadArgs *)args;
  
  struct hostent *hostname = gethostbyname(thread_args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", thread_args->server.ip);
    thread_args->result = 1;
    return NULL;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(thread_args->server.port);
  server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    thread_args->result = 1;
    return NULL;
  }

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Connection to %s:%d failed\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sockfd);
    thread_args->result = 1;
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &thread_args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

  if (send(sockfd, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send to %s:%d failed\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sockfd);
    thread_args->result = 1;
    return NULL;
  }

  char response[sizeof(uint64_t)];
  if (recv(sockfd, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive from %s:%d failed\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sockfd);
    thread_args->result = 1;
    return NULL;
  }

  memcpy(&thread_args->result, response, sizeof(uint64_t));
  close(sockfd);
  
  printf("Server %s:%d returned: %llu for range [%llu, %llu]\n", 
         thread_args->server.ip, thread_args->server.port,
         thread_args->result, thread_args->begin, thread_args->end);
  
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers_file[255] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        break;
      case 2:
        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
        servers_file[sizeof(servers_file) - 1] = '\0';
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  struct Server *servers = NULL;
  int servers_num = ReadServersFromFile(servers_file, &servers);
  if (servers_num <= 0) {
    fprintf(stderr, "No valid servers found in file: %s\n", servers_file);
    return 1;
  }

  printf("Found %d servers\n", servers_num);

  pthread_t threads[servers_num];
  struct ThreadArgs thread_args[servers_num];
  
  uint64_t range_size = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current_start = 1;

  for (int i = 0; i < servers_num; i++) {
    thread_args[i].server = servers[i];
    thread_args[i].mod = mod;
    thread_args[i].begin = current_start;
    
    uint64_t range = range_size;
    if (i < remainder) {
      range++;
    }
    
    thread_args[i].end = current_start + range - 1;
    current_start += range;

    printf("Server %d: %s:%d will compute [%llu, %llu]\n", 
           i, servers[i].ip, servers[i].port, 
           thread_args[i].begin, thread_args[i].end);

    if (pthread_create(&threads[i], NULL, ServerThread, &thread_args[i])) {
      fprintf(stderr, "Error creating thread for server %d\n", i);
      thread_args[i].result = 1;
    }
  }

  uint64_t total_result = 1;
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    total_result = MultModulo(total_result, thread_args[i].result, mod);
  }

  printf("\nFinal result: %llu! mod %llu = %llu\n", k, mod, total_result);

  free(servers);
  return 0;
}