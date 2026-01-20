/*
General Idea of Implementation:

The queue is built from data nodes that hold the data
and waiter nodes that threads are operating in FIFO order.
Every thread recieves a node with its condition variable.
This prevents cases where a new thread will steal an item meant for a waking thread
and restricts threads to wake up in the order they arrived.
*/

#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>


typedef struct node {
    void* data;
    struct node* next;
}Node;

typedef struct waiter_node {
    cnd_t cond;
    struct waiter_node* next;
}WaiterNode;

typedef struct queue {
    Node* dataHead;
    Node* dataTail;
    WaiterNode* waiterHead;
    WaiterNode* waiterTail;
    mtx_t bigLock;
    atomic_size_t visitedCount;
}Queue;

Queue q;

void initQueue(void){
    q.dataHead = NULL;
    q.dataTail = NULL;
    q.waiterHead = NULL;
    q.waiterTail = NULL;
    mtx_init(&q.bigLock, mtx_plain);
    atomic_init(&q.visitedCount, 0);
}

void destroyQueue(void){
    mtx_destroy(&q.bigLock);
}

void enqueue(void* data){
    // step 1: create a new data node
    Node* newNode = malloc(sizeof(Node));
    if (newNode == NULL){
        perror("malloc failed");
        exit(1);
    }
    newNode->data = data;
    newNode->next = NULL;

    // step 2: locking
    mtx_lock(&q.bigLock);// critical section starts here

    //step 3: adding the data node to the queue
    if (q.dataHead == NULL){
        q.dataHead = newNode;
    }
    else {
        q.dataTail->next = newNode;
    }
    q.dataTail = newNode;

    //step 4: waking up the waiter queue
    if (q.waiterHead != NULL){
        cnd_signal(&q.waiterHead->cond);
    }
    //step 5: unlocking
    mtx_unlock(&q.bigLock);// critical section ends here
}

void* dequeue(void){
    //step 1: locking
    mtx_lock(&q.bigLock); // critical section starts here

    //step 2: check if we need to wait
    if (q.dataHead == NULL || q.waiterHead != NULL){
        WaiterNode* newWaiterNode = malloc(sizeof(WaiterNode));
        if (newWaiterNode == NULL){
            perror("malloc failed");
            exit(1);
        }
        cnd_init(&newWaiterNode->cond);
        newWaiterNode->next = NULL;
        if (q.waiterTail == NULL){
            q.waiterHead = newWaiterNode;
            q.waiterTail = newWaiterNode;
        }
        else {
            q.waiterTail->next = newWaiterNode;
            q.waiterTail = newWaiterNode;
        }
        cnd_wait(&newWaiterNode->cond,&q.bigLock);
        //wakeup
        WaiterNode* removedWaiterNode = q.waiterHead;
        q.waiterHead = q.waiterHead->next;
        if (q.waiterHead == NULL) {
            q.waiterTail = NULL;
        }
        cnd_destroy(&removedWaiterNode->cond);
        free(removedWaiterNode);
    }

    //step 3: removing the head
    if (q.dataHead->next == NULL){
        q.dataTail = NULL;
    }
    Node* removedNode = q.dataHead;
    q.dataHead = q.dataHead->next;
    void* ret = removedNode->data;
    free(removedNode);

    //step 4: update stats data
    atomic_fetch_add(&q.visitedCount,1);

    //step 5: unlock and return value
    if (q.dataHead != NULL && q.waiterHead != NULL) {
        cnd_signal(&q.waiterHead->cond);
    }
    mtx_unlock(&q.bigLock);// critical section ends here
    return ret;
}

size_t visited(void){
    return atomic_load(&q.visitedCount);
}