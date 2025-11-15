#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Два мьютекса (замка)
pthread_mutex_t mutex1, mutex2;

// Функция для первого потока
void* thread1(void* arg) {
    printf("Поток 1 лочит mutex1\n");
    pthread_mutex_lock(&mutex1);
    
    sleep(1);
    
    printf("Поток 1 лочит mutex2\n");
    pthread_mutex_lock(&mutex2);
    
    printf("Поток 1 завершил работу\n");
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return NULL;
}

// Функция для второго потока
void* thread2(void* arg) {
    printf("Поток 2 лочит mutex2\n");
    pthread_mutex_lock(&mutex2);
    
    sleep(1);
    
    printf("Поток 2 лочит mutex1\n");
    pthread_mutex_lock(&mutex1);
    
    printf("Поток 2 завершил работу\n");
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return NULL;
}

int main() {
    pthread_t t1, t2;
    
    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);
    
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    
    sleep(3);
    
    printf("DEADLOCK\n");
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    
    return 0;
}