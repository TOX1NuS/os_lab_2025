#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

int main() {
    printf("Создаем зомби-процесс\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        printf("Дочерний: PID=%d завершается\n", getpid());
        exit(0);
    } else {
        printf("Родитель создал дочерний PID=%d\n", pid);
        printf("Родитель НЕ вызывает wait() - дочерний процесс станет зомби\n");
        printf("Родитель спит 10 сек\n");
        
        sleep(10);
        
        printf("Родитель завершается - зомби исчез\n");
    }
    
    return 0;
}