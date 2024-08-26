/*
* scratch_queue: queue implemented on suspensive semaphore
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 1.0
* Uses ss_semaphore library
* 
* Original code by  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*/

// What does this library do?
// This library implements utility methods for working on queues
// For queues initialization see multi_scratch_queue library

#include "scratch_queue.h"
#include <string.h> //for memcpy function

//initialize a queue producer side
void scratch_queue_init_producer(SCRATCH_QUEUE *queue_p, char *m_fifo, int n_items, int token_size, SS_SEMAPHORE producer_sem, SS_SEMAPHORE consumer_sem){
  queue_p->index_read   = queue_p->index_write = 0;
  queue_p->message_size = token_size;
  queue_p->n_items = n_items;
  queue_p->fifo_p = m_fifo;
  queue_p->semaphore_left = producer_sem;
  queue_p->semaphore_used = consumer_sem;
  ss_sem_init(queue_p->semaphore_left, 0);     
  ss_sem_init(queue_p->semaphore_used, 0); 
}

//initialize a queue consumer side
void scratch_queue_init_consumer(SCRATCH_QUEUE *queue_p, char *m_fifo, int n_items, int token_size, SS_SEMAPHORE producer_sem, SS_SEMAPHORE consumer_sem){
  queue_p->index_read = queue_p->index_write = 0;
  queue_p->message_size = token_size;
  queue_p->n_items = n_items;
  queue_p->fifo_p = m_fifo;
  queue_p->semaphore_left = producer_sem;
  queue_p->semaphore_used = consumer_sem;
}

//write on queue specified data; this function has a suspensive semantic
void scratch_queue_write(SCRATCH_QUEUE *queue_p, char *data){
  ss_sem_wait(queue_p->semaphore_left);
  /*write message on queue*/
  memcpy(&queue_p->fifo_p[queue_p->index_write*queue_p->message_size],data,queue_p->message_size);
  /* circular queue */
  ++queue_p->index_write;
  if (queue_p->index_write == queue_p->n_items) queue_p->index_write=0; 
  ss_sem_signal(queue_p->semaphore_used);
}

//the next two function permit to obtain a buffer in the queue and then "commit" it
//this is useful if queue are really allocated on scratch because working in a scratch buffer don't generate bus traffic

//this function permit to obtain the address of the first queue free buffer
//this function has suspensive semantic
char* scratch_queue_getToken_write(SCRATCH_QUEUE *queue_p){
  ss_sem_wait(queue_p->semaphore_left);
  return &(queue_p->fifo_p[queue_p->index_write*queue_p->message_size]);
}

//this function "commit" the last "getToken" buffer in the queue
//must be used only sequentially after scratch_queue_getToken_write
void scratch_queue_putToken_write(SCRATCH_QUEUE *queue_p){
  /* circular queue */
  ++queue_p->index_write;
  if (queue_p->index_write == queue_p->n_items) queue_p->index_write=0; 
  ss_sem_signal(queue_p->semaphore_used); 
}

//read from the queue and write into a buffer; this function has suspensive semantic
void scratch_queue_read(SCRATCH_QUEUE *queue_p, char *buffer){
  ss_sem_wait(queue_p->semaphore_used);
  /*read message from queue*/
  memcpy(buffer,&queue_p->fifo_p[queue_p->index_read*queue_p->message_size],queue_p->message_size);
  /*circular queue */
  ++queue_p->index_read;
  if (queue_p->index_read == queue_p->n_items) queue_p->index_read=0; // circular buffer
  ss_sem_signal(queue_p->semaphore_left);
}
