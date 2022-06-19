#include "../libs/queue.h"
// C program for array implementation of queue

// function to create a queue. It initializes size of queue as 0
struct Queue *createQueue(){
    struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
    queue->capacity = _QUEUE_SIZE_;
    queue->front = queue->size = 0;

    queue->rear = _QUEUE_SIZE_ - 1;
    queue->array = (int*)malloc(queue->capacity * sizeof(int));
    
    return queue;
}

// Queue is full when size becomes equal to the capacity
int isFull(struct Queue *queue){
    return (queue->size == queue->capacity);
}

// Queue is empty when size is 0
int isEmpty(struct Queue *queue){
    return (queue->size == 0);
}

// Function to add an item to the queue. It changes rear and size
void enqueue(struct Queue *queue, int item){
    if (isFull(queue)){
        queue->array=(int*)realloc(queue->array,(queue->capacity)*2);
        queue->capacity*=2;
    }

    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
}

// Function to remove an item from queue. It changes front and size
int dequeue(struct Queue *queue){
    if (isEmpty(queue))
        return -1;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

// Function to get front of queue
int front(struct Queue *queue){
    if (isEmpty(queue))
        return -1;
    return queue->array[queue->front];
}

// Function to get rear of queue
int rear(struct Queue *queue){
    if (isEmpty(queue))
        return -1;
    return queue->array[queue->rear];
}