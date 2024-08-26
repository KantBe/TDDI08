/*
* scratch_queue: queue implemented on suspensive semaphore
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 2.5
* Uses ss_semaphore library
* 
* Original code by  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*/

#define MAXQUEUE 10
#define USEDMA //enable dma use for multiple 32-bit memory transfers
//#define USEINTERRUPT //use ss_semaphore library instead of scratch_sempahore library -> need mparm hw support

#ifdef USEINTERRUPT
  #include "ss_semaphore.h"
#endif

#include "scratch_semaphore.h"

//used to define size of the semaphore in every case (simplify code removing a lot of ifdef macros)
#ifdef USEINTERRUPT
  #define QUEUE_SEMAPHORE_SIZE sizeof(SS_SEMAPHORE)
  #define QUEUE_SEMAPHORE SS_SEMAPHORE
#else
  #define QUEUE_SEMAPHORE_SIZE sizeof(SCRATCH_SEMAPHORE)
  #define QUEUE_SEMAPHORE SCRATCH_SEMAPHORE
#endif

struct scratch_queue_type_producer
{  	
  QUEUE_SEMAPHORE     semaphore_used;
  QUEUE_SEMAPHORE     semaphore_left;
  int                   index_write;
  int		        message_size;	         /* token size in bytes */
  int	  	        n_items;	         /* number of tokens in queue*/  
  char      		*fifo_p;                 /* queue fifo buffer  */
};

struct scratch_queue_type_consumer
{  	
  QUEUE_SEMAPHORE     semaphore_used;
  QUEUE_SEMAPHORE     semaphore_left;
  int                   index_read;
  int		        message_size;	         /* token size in bytes */
  int	  	        n_items;	         /* number of tokens in queue*/  
  char      		*fifo_p;                 /* queue fifo buffer  */
  char			*local_buffer;		 /* consumer self allocated local buffer*/
};

typedef struct scratch_queue_type_producer SCRATCH_QUEUE_PRODUCER;
typedef struct scratch_queue_type_consumer SCRATCH_QUEUE_CONSUMER;

void	scratch_queue_init_producer(SCRATCH_QUEUE_PRODUCER *queue_p, char *m_fifo, int n_items, int token_size,QUEUE_SEMAPHORE producer_sem,QUEUE_SEMAPHORE consumer_sem);
void	scratch_queue_init_consumer(SCRATCH_QUEUE_CONSUMER *queue_c, char *m_fifo, char* local_buffer, int n_items, int token_size,QUEUE_SEMAPHORE producer_sem,QUEUE_SEMAPHORE consumer_sem);
void	scratch_queue_write(SCRATCH_QUEUE_PRODUCER *queue_p, char *data);
char*	scratch_queue_getToken_write(SCRATCH_QUEUE_PRODUCER *queue_p);
void	scratch_queue_putToken_write(SCRATCH_QUEUE_PRODUCER *queue_p);
void	scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_c, char *buffer);
char*	scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_c);

void scratch_queue_autoinit_system(void);
SCRATCH_QUEUE_PRODUCER* scratch_queue_autoinit_producer(int core_consumer_id, int queue_id, int n_items, int token_size);
SCRATCH_QUEUE_CONSUMER* scratch_queue_autoinit_consumer(int queue_id, int allocate_local_buffer);
