#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int start;
    int end;
    long long mod;
    long long partial_result;
    long long *global_result;
    pthread_mutex_t *mutex;
} thread_data_t;

void* compute_partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    long long partial = 1;
    
    printf("Thread: computing from %d to %d\n", data->start, data->end);
    
    for (int i = data->start; i <= data->end; i++) {
        partial = (partial * i) % data->mod;
    }
    
    data->partial_result = partial;
    
    pthread_mutex_lock(data->mutex);
    *(data->global_result) = (*(data->global_result) * partial) % data->mod;
    pthread_mutex_unlock(data->mutex);
    
    printf("Thread: partial result %lld\n", partial);
    
    return NULL;
}

int parse_arguments(int argc, char* argv[], int* k, int* pnum, long long* mod) {
    *k = 10;
    *pnum = 4;
    *mod = 10;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            *k = atoi(argv[i + 1]);
            i++;
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            *pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            *mod = atoll(argv[i] + 6);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s -k <number> --pnum=<threads> --mod=<module>\n", argv[0]);
            return 0;
        }
    }
    
    return 1;
}

int main(int argc, char* argv[]) {
    int k, pnum;
    long long mod;
    long long global_result = 1;
    
    if (!parse_arguments(argc, argv, &k, &pnum, &mod)) {
        return 0;
    }
    
    if (k < 0) {
        printf("Error: k must be non-negative\n");
        return 1;
    }
    
    if (pnum <= 0) {
        printf("Error: pnum must be positive\n");
        return 1;
    }
    
    if (mod <= 0) {
        printf("Error: mod must be positive\n");
        return 1;
    }
    
    if (k == 0 || k == 1) {
        printf("%d! mod %lld = 1\n", k, mod);
        return 0;
    }
    
    printf("Computing %d! mod %lld using %d threads\n", k, mod, pnum);
    
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    
    pthread_t threads[pnum];
    thread_data_t thread_data[pnum];
    
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    for (int i = 0; i < pnum; i++) {
        int numbers_for_this_thread = numbers_per_thread;
        if (i < remainder) {
            numbers_for_this_thread++;
        }
        
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + numbers_for_this_thread - 1;
        thread_data[i].mod = mod;
        thread_data[i].global_result = &global_result;
        thread_data[i].mutex = &mutex;
        
        current_start += numbers_for_this_thread;
        
        if (pthread_create(&threads[i], NULL, compute_partial_factorial, &thread_data[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }
    
    pthread_mutex_destroy(&mutex);
    
    printf("Result: %d! mod %lld = %lld\n", k, mod, global_result);
    
    return 0;
}