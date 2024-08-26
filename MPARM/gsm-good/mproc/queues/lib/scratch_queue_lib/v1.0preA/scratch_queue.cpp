#include "scratch_queue.h"

#include <string.h> //per ma memcpy

void scratch_queue_init_producer(SCRATCH_QUEUE *queue_p, char *m_fifo, int n_items, int token_size, SCRATCH_SEMAPHORE producer_sem, SCRATCH_SEMAPHORE consumer_sem){
  queue_p->index_read   = queue_p->index_write = 0;
  queue_p->message_size = token_size;
  queue_p->n_items = n_items;
  queue_p->fifo_p = m_fifo;
  queue_p->semaphore_left = producer_sem;
  queue_p->semaphore_used = consumer_sem;
 
  scratch_sem_init(queue_p->semaphore_left, 0);     
  scratch_sem_init(queue_p->semaphore_used, 0); 
}

void scratch_queue_init_consumer(SCRATCH_QUEUE *queue_p, char *m_fifo, int n_items, int token_size, SCRATCH_SEMAPHORE producer_sem, SCRATCH_SEMAPHORE consumer_sem){
  queue_p->index_read = queue_p->index_write = 0;
  queue_p->message_size = token_size;
  queue_p->n_items = n_items;
  queue_p->fifo_p = m_fifo;

  queue_p->semaphore_left = producer_sem;
  queue_p->semaphore_used = consumer_sem;
}

void scratch_queue_write(SCRATCH_QUEUE *queue_p, char *data){

  while(scratch_queue_peek_w(queue_p)==0);

  /*write message on queue*/
  memcpy(&queue_p->fifo_p[queue_p->index_write*queue_p->message_size],data,queue_p->message_size);
  
  /* circular queue */
  ++queue_p->index_write;
  if (queue_p->index_write == queue_p->n_items) queue_p->index_write=0; 
  
  scratch_sem_signal(queue_p->semaphore_used); 
}

char*	scratch_queue_getToken_write(SCRATCH_QUEUE *queue_p){
  while(scratch_queue_peek_w(queue_p)==0);
  return &(queue_p->fifo_p[queue_p->index_write*queue_p->message_size]);
}

void	scratch_queue_putToken_write(SCRATCH_QUEUE *queue_p){
  /* circular queue */
  ++queue_p->index_write;
  if (queue_p->index_write == queue_p->n_items) queue_p->index_write=0; 
  
  scratch_sem_signal(queue_p->semaphore_used); 
}

void	scratch_queue_read(SCRATCH_QUEUE *queue_p, char *buffer){

  while(scratch_queue_peek_r(queue_p)==0);

  /*read message from queue*/
  memcpy(buffer,&queue_p->fifo_p[queue_p->index_read*queue_p->message_size],queue_p->message_size);
  
  /*circular queue */
  ++queue_p->index_read;
  if (queue_p->index_read == queue_p->n_items) queue_p->index_read=0; // circular buffer
  
  scratch_sem_signal(queue_p->semaphore_left);
}

/*===========  scratch_queue_peek_w  ============================
*
*   Non blocking check for available writing space on the queue
*   
*   Return the number of free tokens (0 if the queue is full)
*/

inline int scratch_queue_peek_w(SCRATCH_QUEUE *queue_p){ return(scratch_sem_wait(queue_p->semaphore_left)); }


/*===========  scratch_queue_peek_r  ============================
*
*   Non blocking check for available data on the queue
*   
*   Return the number of ready tokens in the queue (0 if the queue is empty)
*/

inline int scratch_queue_peek_r(SCRATCH_QUEUE *queue_p){ return(scratch_sem_wait(queue_p->semaphore_used)); }

/*===========  scratch_queue_len  ============================
*
*   Non blocking check for queue length.
*   
*   Return the number of ready tokens in the queue (0 if the queue is empty)
*
*   WITHOUT decrementing the value 
*/

inline int scratch_queue_len(SCRATCH_QUEUE *queue_p){
  int length;
   length=scratch_sem_wait(queue_p->semaphore_used);
  if (length!=0) scratch_sem_signal(queue_p->semaphore_used);
  return(length);
}
