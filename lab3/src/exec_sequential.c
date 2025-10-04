#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    else if (pid == 0) {
        printf("Child process (PID: %d) is starting sequential_min_max\n", getpid());
        
        char *args[] = {"./sequential_min_max", argv[1], argv[2], NULL};
        
        execvp("./sequential_min_max", args);
        
        perror("execvp failed");
        exit(1);
    }
    else {
        printf("Parent process (PID: %d) created child process (PID: %d)\n", 
               getpid(), pid);
    }
    
    return 0;
}