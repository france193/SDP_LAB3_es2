/**
 * Name:    Francesco
 * Surname: Longo
 * ID:      223428
 * Lab:     3
 * Ex:      2
 *
 * Write a concurrent C program using only conditions and mutexes, that implement the Readers & Writers protocol,
 * with precedence to the Readers.
 * In particular, the main threads creates N readers and N writers (N given as an argument of the command line)
 * and waits for their termination.
 *
 * Each reader/writer:
 * - sleeps for a random time of millisecond (0-500)
 * - printf(“Thread %d trying to read/write at time %d\n”, ...)
 * - when it is able to read, it printf(“Thread %d reading/writing at time %d1n”, ... )
 * - simulates the reading/writing operation sleeping 500 milliseconds
 * - terminates
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

sem_t *mutual_exclusion_reader, *mutual_exclusion_writer, *reader_priority;
int nr = 0;

static void *writer(void *a){
    int *i = (int *) a;

    sleep(rand()%2);
    printf("Writer %d trying to write\n", *i);

    sem_wait(mutual_exclusion_writer);
    sem_wait(reader_priority);

    printf("Thread n. %d writing\n", *i);
    sleep(3);

    sem_post(reader_priority);
    sem_post(mutual_exclusion_writer);

    return NULL;
}

static void *reader(void *a){
    int *i = (int *) a;

    sleep(rand()%10);
    printf("Reader %d trying to read\n", *i);

    sem_wait(mutual_exclusion_reader);
    // new reader
    nr++;
    if (nr == 1){
        // if there is at least a reader, block writers
        sem_wait(reader_priority);
    }
    sem_post(mutual_exclusion_reader);

    printf("Thread n. %d reading\n", *i);
    sleep(1);

    sem_wait(mutual_exclusion_reader);
    // reader finishes
    nr--;

    if(nr == 0) {
        // if there is no more readers free writer access
        sem_post (reader_priority);
    }
    sem_post (mutual_exclusion_reader);

    return NULL;
}

int main(void){
    pthread_t th_a, th_b;
    int i, *v;

    // allocate and init semaphores
    reader_priority = (sem_t *) malloc (sizeof (sem_t));
    mutual_exclusion_reader = (sem_t *) malloc (sizeof (sem_t));
    mutual_exclusion_writer = (sem_t *) malloc (sizeof (sem_t));
    sem_init(reader_priority, 0, 1);
    sem_init(mutual_exclusion_reader, 0, 1);
    sem_init(mutual_exclusion_writer, 0, 1);

    setbuf(stdout,0);

    // Create the threads
    for (i = 0; i<10; i++) {
        v = (int *) malloc (sizeof (int));
        *v = i;
        pthread_create(&th_a, NULL, reader, v);
    }

    for (i = 0; i < 10; i++) {
        v = (int *) malloc (sizeof (int));
        *v = i;
        pthread_create(&th_b, NULL, writer, v);
    }

    pthread_exit(0);
}

int getRandomNumber(unsigned int seed, int min, int max) {
    //srand(seed);
    int num = rand() % (max - min + 1) + min;
    return num;
}

void nano_sleep(long  ns) {
    struct timespec tim, tim_remaining;

    tim.tv_sec = 0;
    tim.tv_nsec = ns;   // 0 to  999999999 nanosecond
    if (nanosleep(&tim, &tim_remaining) == -1 && errno == EINTR){
        fprintf(stderr, "nanosleep interrupted\n");
        exit(1);
    }
}

int millisleep(long milliseconds) {
    struct timespec req, rem;

    if(milliseconds > 999) {
        req.tv_sec = (int)(milliseconds / 1000);                            /* Must be Non-Negative */
        req.tv_nsec = (milliseconds - ((long)req.tv_sec * 1000)) * 1000000; /* Must be in range of 0 to 999999999 */
    } else {
        req.tv_sec = 0;                         /* Must be Non-Negative */
        req.tv_nsec = milliseconds * 1000000;    /* Must be in range of 0 to 999999999 */
    }

    return nanosleep(&req , &rem);
}
