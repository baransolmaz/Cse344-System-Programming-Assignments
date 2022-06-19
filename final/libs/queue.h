#if !defined(QUEUE_H)
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _QUEUE_SIZE_ 100
struct Queue
{
    int front, rear, size;
    int capacity;
    int *array;
};

int dequeue(struct Queue *queue);            // To get first element
int front(struct Queue *queue);              // Function to get front of queue
int rear(struct Queue *queue);               // Function to get rear of queue
int isEmpty(struct Queue *queue);            // Checks queue if it empty return 1
int isFull(struct Queue *queue);             // Checks queue if it full return 1
struct Queue *createQueue();                 // Constructor
void enqueue(struct Queue *queue, int item); // To insert element
#endif // QUEUE_H