/*
* scratch_queue: queue implemented on suspensive semaphore
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 2.1
* Uses ss_semaphore library
* 
* Original code by  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*/

#include "ss_semaphore.h"

struct scratch_queue_type_producer
{  	
  SS_SEMAPHORE     semaphore_used;     
  SS_SEMAPHORE     semaphore_left;     
  int                   index_write;
  int		        message_size;	         /* token size in bytes */
  int	  	        n_items;	         /* number of tokens in queue*/  
  char      		*fifo_p;                 /* queue fifo buffer  */
};

struct scratch_queue_type_consumer
{  	
  SS_SEMAPHORE     semaphore_used;     
  SS_SEMAPHORE     semaphore_left;     
  int                   index_read;
  int		        message_size;	         /* token size in bytes */
  int	  	        n_items;	         /* number of tokens in queue*/  
  char      		*fifo_p;                 /* queue fifo buffer  */
  char			*local_buffer;		 /* consumer self allocated local buffer*/
};

typedef struct scratch_queue_type_producer SCRATCH_QUEUE_PRODUCER;
typedef struct scratch_queue_type_consumer SCRATCH_QUEUE_CONSUMER;

void	scratch_queue_init_producer(SCRATCH_QUEUE_PRODUCER *queue_p, char *m_fifo, int n_items, int token_size,SS_SEMAPHORE producer_sem,SS_SEMAPHORE consumer_sem);
void	scratch_queue_init_consumer(SCRATCH_QUEUE_CONSUMER *queue_c, char *m_fifo, char* local_buffer, int n_items, int token_size,SS_SEMAPHORE producer_sem,SS_SEMAPHORE consumer_sem);
void	scratch_queue_write(SCRATCH_QUEUE_PRODUCER *queue_p, char *data);
char*	scratch_queue_getToken_write(SCRATCH_QUEUE_PRODUCER *queue_p);
void	scratch_queue_putToken_write(SCRATCH_QUEUE_PRODUCER *queue_p);
void	scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_c, char *buffer);
char*	scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_c);
