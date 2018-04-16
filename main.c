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

pthread_mutex_t mutex;
pthread_cond_t turn;
int writing;
int reading;
int writers;

void init(void);
static void *writer(void);
static void *reader(void);
int getRandomNumber(unsigned int seed, int min, int max) ;
int millisleep(long milliseconds) ;

long long current_timestamp() ;

// main
int main(int argc, char **argv) {
    pthread_t *th_readers, *th_writers;
    int n;

    printf(" > Main started!\n");

    if (argc != 2) {
        fprintf(stdout, "Expected 2 argument: <prog_name> <n>\n");
        exit(-1);
    }

    n = atoi(argv[1]);
    printf(" > N is %d!\n", n);

    th_readers = (pthread_t *)malloc(n*sizeof(pthread_t));
    th_writers = (pthread_t *)malloc(n*sizeof(pthread_t));

    // allocate and init condition and mutex
    init();

    setbuf(stdout, 0);

    // Create n readers
    for (int i = 0; i < n; i++) {
        pthread_create(&th_readers[i], NULL, (void *(*)(void *)) reader, NULL);
    }

    // Create n writers
    for (int i = 0; i < n; i++) {
        pthread_create(&th_writers[i], NULL, (void *(*)(void *)) writer, NULL);
    }

    // wait n readers
    for (int i = 0; i < n; i++) {
        printf(" > A reader is terminated!\n");
        pthread_join(th_writers[i], NULL);
    }

    // wait n writers
    for (int i = 0; i < n; i++) {
        printf(" > A writer is terminated!\n");
        pthread_join(th_readers[i], NULL);
    }

    printf(" > Main exiting...\n");
    pthread_exit(0);
}

// functions
static void *writer(void){
    int rand_time = getRandomNumber((unsigned int) time(NULL), 0, 500);
    millisleep(rand_time);
    printf(" >> Thread %d trying to write at time %lli\n", (int) pthread_self(), current_timestamp());

    pthread_mutex_lock(&mutex);
    writers++;
    while (reading || writing) {
        pthread_cond_wait(&turn, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    /* WRITE */
    printf(" >> Thread %d writing at time %lli\n", (int) pthread_self(), current_timestamp());
    millisleep(500);

    pthread_mutex_lock(&mutex);
    writing--;
    writers--;
    pthread_cond_broadcast(&turn);
    pthread_mutex_unlock(&mutex);

    pthread_exit(0);
}

static void *reader(void){
    int rand_time = getRandomNumber((unsigned int) time(NULL), 0, 500);
    millisleep(rand_time);
    printf(" >>> Thread %d trying to read at time %lli\n", (int) pthread_self(), current_timestamp());

    pthread_mutex_lock(&mutex);
    if (writers) {
        pthread_cond_wait(&turn, &mutex);
    }
    while (writing) {
        pthread_cond_wait(&turn, &mutex);
    }
    reading++;
    pthread_mutex_unlock(&mutex);

    /* READ */
    printf(" >>> Thread %d reading at time %lli\n", (int) pthread_self(), current_timestamp());
    millisleep(500);

    pthread_mutex_lock(&mutex);
    reading--;
    pthread_cond_broadcast(&turn);
    pthread_mutex_unlock(&mutex);

    pthread_exit(0);
}

void init(void) {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&turn, NULL);
    reading = 0;
    writing = 0;
    writers = 0;
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

long long current_timestamp() {
    struct timeval te;

    // get current time
    gettimeofday(&te, NULL);

    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;

    return milliseconds;
}
