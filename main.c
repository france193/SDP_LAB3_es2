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

// constants
#define NORMAL 0
#define URGENT 1
#define BUFFER_SIZE 16
#define LOOP_TIMES 100

// struct
typedef struct Data {
    int buf;
    long long ms;
} Data;

typedef struct Buffer {
    sem_t *empty;
    sem_t *full;
    sem_t *mutual_exclusion_producer;
    sem_t *mutual_exclusion_consumer;
    int in;
    int out;
    Data *data;
    int size;
} Buffer;

// global variables
Buffer *normal_buf;
Buffer *urgent_buf;
float probability;

// prototypes
long long current_timestamp();
void buffer_init(int size);
void *consumer(void);
void *producer(void);
int getRandomNumber(unsigned int seed, int min, int max);
int millisleep(long milliseconds);
void nano_sleep(long  ns);
void putOnBuffer(Data data, Buffer *b);
Data getFromBuffer();

// main
int main(int argc, char **argv) {
    printf(" > Main started!\n");

    if (argc != 2) {
        fprintf(stdout, "Expected 2 argument: <prog_name> <p>\n");
        exit(-1);
    }

    probability = (float) atof(argv[1]);
    printf(" > Probability: %f!\n", probability);

    // variables
    pthread_t th_producer, th_consumer;
    void *retval;

    buffer_init(BUFFER_SIZE);
    printf(" > Buffers created!\n");

    // create threads
    pthread_create(&th_consumer, NULL, (void *(*)(void *)) consumer, 0);
    pthread_create(&th_producer, NULL, (void *(*)(void *)) producer, 0);
    printf(" > Thread created!\n");

    // wait until producer and consumer end
    printf(" > Waiting for thread termination...\n");
    pthread_join(th_consumer, &retval);
    pthread_join(th_producer, &retval);
    printf(" > Threads terminated! ---> Terminating main...\n");

    return 0;
}

// threads
void *consumer(void) {
    // loops 100 times
    for (int i = 0; i<LOOP_TIMES; i++) {
        millisleep(10);

        // get data from buffer
        Data data = getFromBuffer();

        // print information
        if (data.buf == URGENT) {
            printf(" >>> Consumer: ms is %lli and buffer is URGENT!\n", data.ms);
        } else {
            printf(" >>> Consumer: ms is %lli and buffer is NORMAL!\n", data.ms);
        }

    }
    return NULL;
}

void *producer(void) {
    // loops 100 times
    for (int i = 0; i<LOOP_TIMES; i++) {
        // sleeps between 1 and 10 ms
        long rand = getRandomNumber((unsigned int) time(NULL), 1, 10);
        millisleep(rand);

        // get current time in ms
        long long ms = current_timestamp();

        // randomly selects normal or urgent buffer
        int int_probability = (int) (probability * 10);
        //printf(" >> Producer: int_probability is %i!\n", int_probability);

        int rand_n = getRandomNumber((unsigned int) time(NULL), 1, 10);
        //printf(" >> Producer: rand_n is %i!\n", rand_n);

        Buffer *b;
        int buf;
        if (rand_n < int_probability) {
            b = normal_buf;
            buf = NORMAL;
        } else {
            b = urgent_buf;
            buf = URGENT;
        }

        Data d;

        // print data
        if (buf == URGENT) {
            printf(" >> Producer: ms is %lli and buffer is URGENT!\n", ms);
            d.buf = URGENT;
        } else {
            printf(" >> Producer: ms is %lli and buffer is NORMAL!\n", ms);
            d.buf = NORMAL;
        }
        d.ms = ms;

        // put data inside buffer
        putOnBuffer(d, b);
    }
    return NULL;
}

// functions
long long current_timestamp() {
    struct timeval te;

    // get current time
    gettimeofday(&te, NULL);

    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;

    return milliseconds;
}

void buffer_init(int size) {
    // allocate a new urgent_buffer
    urgent_buf = (Buffer *)malloc(sizeof(Buffer));

    // initialization
    urgent_buf->size = size;
    urgent_buf->data = (Data *)malloc(urgent_buf->size * sizeof(Data));
    urgent_buf->in = 0;
    urgent_buf->out = 0;

    // semaphores allocation
    urgent_buf->empty = (sem_t *)malloc(sizeof(sem_t));
    urgent_buf->full = (sem_t *)malloc(sizeof(sem_t));
    urgent_buf->mutual_exclusion_consumer = (sem_t *)malloc(sizeof(sem_t));
    urgent_buf->mutual_exclusion_producer = (sem_t *)malloc(sizeof(sem_t));

    // semaphores initalization
    sem_init(urgent_buf->empty, 0, (unsigned int) urgent_buf->size);
    sem_init(urgent_buf->full, 0, 0);
    sem_init(urgent_buf->mutual_exclusion_consumer, 0, 1);
    sem_init(urgent_buf->mutual_exclusion_producer, 0, 1);

    // allocate a new normal_buffer
    normal_buf = (Buffer *)malloc(sizeof(Buffer));

    // initialization
    normal_buf->size = size;
    normal_buf->data = (Data *)malloc(urgent_buf->size * sizeof(Data));
    normal_buf->in = 0;
    normal_buf->out = 0;

    // semaphores allocation
    normal_buf->empty = (sem_t *)malloc(sizeof(sem_t));
    normal_buf->full = (sem_t *)malloc(sizeof(sem_t));
    normal_buf->mutual_exclusion_consumer = (sem_t *)malloc(sizeof(sem_t));
    normal_buf->mutual_exclusion_producer = (sem_t *)malloc(sizeof(sem_t));

    // semaphores initalization
    sem_init(normal_buf->empty, 0, (unsigned int) normal_buf->size);
    sem_init(normal_buf->full, 0, 0);
    sem_init(normal_buf->mutual_exclusion_consumer, 0, 1);
    sem_init(normal_buf->mutual_exclusion_producer, 0, 1);
}

void putOnBuffer(Data data, Buffer *b) {
    sem_wait(b->empty);
    b->data[b->in] = data;
    // increment counter of data inside buffer
    b->in = (b->in + 1) % b->size;
    sem_post(b->full);
}

Data getFromBuffer() {
    Data data;
    int result, value;
    Buffer *b;

    // select buffer with priority
    if ((result = sem_getvalue(urgent_buf->full, &value) == -1)) {
        fprintf(stdout, "Error retrieving sem value\n");
        exit(-1);
    }

    if (value > 0) {
        b = urgent_buf;
    } else {
        b = normal_buf;
    }

    sem_wait(b->full);
    data.buf = b->data[b->out].buf;
    data.ms = b->data[b->out].ms;
    // decrement counter of data inside buffer
    b->out = (b->out + 1) % b->size;
    sem_post(b->empty);

    return data;
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
