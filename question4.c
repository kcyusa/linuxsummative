#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_QUEUE_SIZE 5   // Maximum items in queue

// ===============================
// Shared Data Structures
// ===============================
int queue[MAX_QUEUE_SIZE];   // Shared assembly line (circular buffer)
int front = 0, rear = 0, count = 0; // Queue tracking

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_producer = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_consumer = PTHREAD_COND_INITIALIZER;

/*
==========================================================
 Thread Synchronization Logic:
----------------------------------------------------------
 - Mutex protects shared queue operations (push/pop).
 - Condition variables control producer/consumer waits:
     * Producer waits if queue is full.
     * Consumer waits if queue is empty.
 - Signals are used to wake up threads when state changes.
==========================================================
*/

// ===============================
// Queue Operations
// ===============================
void enqueue(int item) {
    queue[rear] = item;
    rear = (rear + 1) % MAX_QUEUE_SIZE;
    count++;
}

int dequeue() {
    int item = queue[front];
    front = (front + 1) % MAX_QUEUE_SIZE;
    count--;
    return item;
}

// ===============================
// Producer Thread
// ===============================
void* producer(void* arg) {
    int item = 1;

    while (1) {
        pthread_mutex_lock(&mutex);

        // Wait if queue is full
        while (count == MAX_QUEUE_SIZE) {
            printf("[Producer] Queue full. Waiting...\n");
            pthread_cond_wait(&cond_producer, &mutex);
        }

        // Produce and add item
        enqueue(item);
        printf("[Producer] Produced item %d | Queue size: %d\n", item, count);
        item++;

        // Signal consumer that an item is available
        pthread_cond_signal(&cond_consumer);
        pthread_mutex_unlock(&mutex);

        sleep(2); // Simulate production time
    }
    return NULL;
}

// ===============================
// Consumer Thread
// ===============================
void* consumer(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);

        // Wait if queue is empty
        while (count == 0) {
            printf("[Consumer] Queue empty. Waiting...\n");
            pthread_cond_wait(&cond_consumer, &mutex);
        }

        // Consume item
        int item = dequeue();
        printf("[Consumer] Consumed item %d | Queue size: %d\n", item, count);

        // Signal producer that space is available
        pthread_cond_signal(&cond_producer);
        pthread_mutex_unlock(&mutex);

        sleep(3); // Simulate packaging time
    }
    return NULL;
}

// ===============================
// Main: Start Producer & Consumer Threads
// ===============================
int main() {
    pthread_t producer_thread, consumer_thread;

    // Create threads
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    // Wait for threads (infinite simulation)
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    return 0;
}

/*
==========================================================
 Shared Memory Management:
----------------------------------------------------------
 - queue[] is shared between threads.
 - Access is controlled by mutex to prevent race conditions.
 - Circular buffer logic ensures proper enqueue/dequeue.

 Behavior:
----------------------------------------------------------
 - If producer tries to add when queue is full, it waits.
 - If consumer tries to remove when queue is empty, it waits.
 - Threads wake each other up using condition variables.
==========================================================
*/
