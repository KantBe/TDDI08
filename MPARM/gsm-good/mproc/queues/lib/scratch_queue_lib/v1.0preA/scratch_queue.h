/**************************HEADER INFORMATIONS***************************/
/*
*    DATE          :  27/02/2004
*                            
*    AUTHORS       :  Francesco Menichelli(menichelli@die.uniroma1.it) 
*                     Fabio Stirpe(strfab@libero.it)
*                   
*    VERSION       :  1.0
*                                              
*    CONTENTS      :  Prototypes and defines of
*                     queues in scratch memory
*/

#include "scratch_semaphore.h"

struct scratch_queue_type 
{  	
  SCRATCH_SEMAPHORE     semaphore_used;     
  SCRATCH_SEMAPHORE     semaphore_left;     
  int                   index_read,index_write;
  int		        message_size;	         /* token size in bytes */
  int	  	        n_items;	         /* number of tokens in queue*/  
  char      		*fifo_p;                 /* queue fifo buffer  */
};

typedef struct scratch_queue_type SCRATCH_QUEUE;

void	scratch_queue_init_producer(SCRATCH_QUEUE *queue_p, char *m_fifo, int n_items, int token_size,SCRATCH_SEMAPHORE producer_sem,SCRATCH_SEMAPHORE consumer_sem);
void	scratch_queue_init_consumer(SCRATCH_QUEUE *queue_p, char *m_fifo, int n_items, int token_size,SCRATCH_SEMAPHORE producer_sem,SCRATCH_SEMAPHORE consumer_sem);
void	scratch_queue_write(SCRATCH_QUEUE *queue_p, char *data);
char*	scratch_queue_getToken_write(SCRATCH_QUEUE *queue_p);
void	scratch_queue_putToken_write(SCRATCH_QUEUE *queue_p);
void	scratch_queue_read(SCRATCH_QUEUE *queue_p, char *buffer);

int	scratch_queue_peek_r(SCRATCH_QUEUE *queue_p);
int	scratch_queue_peek_w(SCRATCH_QUEUE *queue_p);
int	scratch_queue_len(SCRATCH_QUEUE *queue_p);
