#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <semaphore.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "os_list.h"

#define MAX_TASK 100
#define MAX_THREAD 4

int sum = 0;
int checkIfTasksAreDone = 0;
os_graph_t *graph;
os_threadpool_t *tp;
pthread_mutex_t mutex;

int processingIsDone(os_threadpool_t *tp) {
    // Check if all nodes were visited
    for (int i = 0; i < graph->nCount; i++) {
        if (graph->visited[i] == 0) {
            return 0;
        }
    }

    // Check if all tasks were executed
    if (checkIfTasksAreDone == graph->nCount) {
        return 1;
    } else {
        return 0;
    }
}

void processNode(unsigned int nodeIdx) {
    // Processing the node is increasing the sum with the node's value
    // and then it gets through all neighbours and creates tasks for them
    // I added a mutex because just one thread have to modify the sum
    // at a time.
    os_node_t *node = graph->nodes[nodeIdx];
    pthread_mutex_lock(&mutex);
    sum += node->nodeInfo;
    checkIfTasksAreDone++;
    int i = 0;
    while (i < node->cNeighbours) {
        if (graph->visited[node->neighbours[i]] == 0) {
            graph->visited[node->neighbours[i]] = 1;
            os_task_t *task = task_create((void *)(long)node->neighbours[i], (void *)processNode);
            add_task_in_queue(tp, task);
        }
        i++;
    }
    pthread_mutex_unlock(&mutex);
}

void traverse_graph() {
    // Here I create tasks just for the first node and for the ones that
    // has no neighbours(because they will not be visited by processNode)
    // and are not visited.
    for (int i = 0; i < graph->nCount; i++) {
        if (i == 0 && graph->visited[i] == 0) {
            graph->visited[i] = 1;
            os_task_t *task = task_create((void *)(long)i, (void *)processNode);
            add_task_in_queue(tp, task);
        }
        if (graph->visited[i] == 0 && graph->nodes[i]->cNeighbours == 0) {
            graph->visited[i] = 1;
            os_task_t *task = task_create((void *)(long)i, (void *)processNode);
            add_task_in_queue(tp, task);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./main input_file\n");
        exit(1);
    }

    FILE *input_file = fopen(argv[1], "r");

    if (input_file == NULL) {
        printf("[Error] Can't open file\n");
        return -1;
    }

    graph = create_graph_from_file(input_file);
    if (graph == NULL) {
        printf("[Error] Can't read the graph from file\n");
        return -1;
    }

    // Creating the threadpool and testing it with traverse_graph
    pthread_mutex_init(&mutex, NULL);
    tp = threadpool_create(MAX_TASK, MAX_THREAD);
    traverse_graph();
    threadpool_stop(tp, processingIsDone);
    printf("%d", sum);
    return 0;
}
