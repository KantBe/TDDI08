/*
* scratch_queue: queue implemented on suspensive semaphore
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 1.0
* Uses ss_semaphore library
* 
* Original code by  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*/

#include "ss_semaphore.h"

struct scratch_queue_type 
{  	
  SS_SEMAPHORE     semaphore_used;     
  SS_SEMAPHORE     semaphore_left;     
  int                   index_read,index_write;
  int		        message_size;	         /* token size in bytes */
  int	  	        n_items;	         /* number of tokens in queue*/  
  char      		*fifo_p;                 /* queue fifo buffer  */
};

typedef struct scratch_queue_type SCRATCH_QUEUE;

void	scratch_queue_init_producer(SCRATCH_QUEUE *queue_p, char *m_fifo, int n_items, int token_size,SS_SEMAPHORE producer_sem,SS_SEMAPHORE consumer_sem);
void	scratch_queue_init_consumer(SCRATCH_QUEUE *queue_p, char *m_fifo, int n_items, int token_size,SS_SEMAPHORE producer_sem,SS_SEMAPHORE consumer_sem);
void	scratch_queue_write(SCRATCH_QUEUE *queue_p, char *data);
char*	scratch_queue_getToken_write(SCRATCH_QUEUE *queue_p);
void	scratch_queue_putToken_write(SCRATCH_QUEUE *queue_p);
void	scratch_queue_read(SCRATCH_QUEUE *queue_p, char *buffer);
