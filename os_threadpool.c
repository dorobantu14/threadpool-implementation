#include "os_threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* === TASK === */

/* Creates a task that thread must execute */
os_task_t *task_create(void *arg, void (*f)(void *)) {
    os_task_t *new_task = malloc(sizeof(os_task_t));

    if (new_task != NULL) {
        new_task->argument = arg;
        new_task->task = f;
    }
    return new_task;
}

// Function for getting the length of task queue
int getCount(os_task_queue_t *head) {
    int count = 0;
    os_task_queue_t *current = head;

    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

/* Add a new task to threadpool task queue */
void add_task_in_queue(os_threadpool_t *tp, os_task_t *t) {
    os_task_queue_t *new_queue_task = (os_task_queue_t *)malloc(sizeof(os_task_queue_t));

    if (new_queue_task == NULL)
        return;

    new_queue_task->task = t;
    new_queue_task->next = NULL;

    pthread_mutex_lock(&tp->taskLock);
    // Check if the queue is empty, if yes than the head of it
    // will be the new node, else we will get through the list
    // and will add the new node at the end of it
    if (tp->tasks == NULL) {
        tp->tasks = new_queue_task;
    } else {
        os_task_queue_t *current_node = tp->tasks;
        while (current_node->next != NULL)  {
            current_node = current_node->next;
        }
        current_node->next = new_queue_task;
    }
    pthread_mutex_unlock(&tp->taskLock);
}

/* Get the head of task queue from threadpool */
os_task_t *get_task(os_threadpool_t *tp) {
    return tp->tasks->task;
}

/* === THREAD POOL === */

/* Initialize the new threadpool */
os_threadpool_t *threadpool_create(unsigned int nTasks, unsigned int nThreads) {
    os_threadpool_t *new_threadpool = (os_threadpool_t *)malloc(sizeof(os_threadpool_t));

    new_threadpool->threads = malloc(nThreads * sizeof(pthread_t));
    new_threadpool->num_threads = nThreads;
    new_threadpool->should_stop = 0;
    pthread_mutex_init(&new_threadpool->taskLock, NULL);
    for (int i = 0; i < nThreads; i++) {
        pthread_create(&(new_threadpool->threads[i]), NULL, thread_loop_function, (void *)new_threadpool);
    }

    return new_threadpool;
}

/* Loop function for threads */
void *thread_loop_function(void *args) {
    os_threadpool_t *tp = (os_threadpool_t *)args;
    
    while (tp->should_stop == 0) {
        pthread_mutex_lock(&tp->taskLock);
        // Check if there is a task in the queue
        if (tp->tasks != NULL) {
            // Dequeue the next task from the queue
            os_task_t *taskToExectute = get_task(tp);

            // Remove the task node from the queue
            tp->tasks = tp->tasks->next;

            pthread_mutex_unlock(&tp->taskLock);

            // Execute the task function
            taskToExectute->task(taskToExectute->argument);
        } else {
            pthread_mutex_unlock(&tp->taskLock);
        }
    }
    return 0;
}

/* Stop the thread pool once a condition is met */
void threadpool_stop(os_threadpool_t *tp, int (*processingIsDone)(os_threadpool_t *)) {
    // Waiting until the tasks are finished
    while (!processingIsDone(tp)) {}

    if (processingIsDone(tp) == 1) {
        tp->should_stop = 1;
        for (int i = 0; i < tp->num_threads; i++) {
            pthread_join(tp->threads[i], NULL);
        }
    }

    // Free the memory used by the thread pool and its tasks
    while (tp->tasks != NULL) {
        os_task_queue_t *next_node = tp->tasks->next;
        free(tp->tasks->task);
        free(tp->tasks);
        tp->tasks = next_node;
    }

    free(tp->threads);
    free(tp);
}
