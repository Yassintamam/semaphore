#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 5  // Number of mCounter threads
#define BUFFER_SIZE 5

int counter = 0;

sem_t counterCTRL, bufferOCCUPIED, bufferAVAILABLE;

void *mCounter(void *arg) {
    int threadID = *((int *)arg);

    while (1) {
        sleep(2);   //sleep till messages has arrived

        printf("Counter thread %d: received a message\n", threadID);

        sem_wait(&counterCTRL);
        printf("Counter thread %d: waiting to write\n", threadID);
        counter++;
        printf("Counter thread %d: now adding to counter, counter value=%d\n", threadID, counter);
        sem_post(&counterCTRL); // signal that counter is in use by mcounter thread

        sem_post(&bufferOCCUPIED);  // Signal that a message is available
    }

    return NULL;
}

void *mMonitor(void *arg) {
    while (1) {
        sleep(2);

        sem_wait(&bufferOCCUPIED);  // Wait for a message to be available
        sem_wait(&counterCTRL);
        printf("Monitor thread: reading a count value of %d\n", counter);
        int valueToWrite = counter;
        counter = 0;
        sem_post(&counterCTRL);

        sem_post(&bufferAVAILABLE);  // Signal that the critical section is available for the collector
    }

    return NULL;
}

void *mCollector(void *arg) {
    while (1) {
        sleep(2);

        sem_wait(&bufferAVAILABLE);  // Wait for the critical section to be available
        sem_wait(&counterCTRL);   //waiting for the counter to be available for use
        printf("Collector thread: reading from the counter\n");
        sem_post(&counterCTRL);
    }

    return NULL;
}

int main() {
    srand(time(NULL));

    sem_init(&counterCTRL, 0, 1);
    sem_init(&bufferOCCUPIED, 0, 0);
    sem_init(&bufferAVAILABLE, 0, BUFFER_SIZE);

    pthread_t counterThreads[N];
    pthread_t monitorThread, collectorThread;

    int counterThreadIDs[N];

    for (int i = 0; i < N; i++) {
        counterThreadIDs[i] = i;
        pthread_create(&counterThreads[i], NULL, mCounter, &counterThreadIDs[i]);
    }

    pthread_create(&monitorThread, NULL, mMonitor, NULL);
    pthread_create(&collectorThread, NULL, mCollector, NULL);

    //joining threads to avoid clashes
    for (int i = 0; i < N; i++) {
        pthread_join(counterThreads[i], NULL);
    }
    pthread_join(monitorThread, NULL);
    pthread_join(collectorThread, NULL);

    sem_destroy(&counterCTRL);
    sem_destroy(&bufferOCCUPIED);
    sem_destroy(&bufferAVAILABLE);

    return 0;
}
