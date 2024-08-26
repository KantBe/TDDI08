/*
* scratch_queue: queue implemented on suspensive semaphore
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 2.8
* Uses ss_semaphore library
* 
* Original code by  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*/
#include <bsp.h>
#include "config.h" //scratch base + queue base + MPARM configuration parameters

#ifndef SCRATCH_QUEUE_H
#define SCRATCH_QUEUE_H

#define MAXQUEUE 10
//#define USEDMA //enable dma use for multiple 32-bit memory transfers
//#define USEINTERRUPT //use ss_semaphore library instead of scratch_sempahore library -> need mparm hw support

//#define DEBUG0 //prints on serial ports initialization procedure results
//#define DEBUG1 //prints on serial ports producer/consumer procedure results
//#define DEBUGSWI //prints via swi debug informations during work
#include "appsupport.h"
//#define DEBUGSWIDMA //prints via swi debug informations about DMA work

#ifdef DEBUG0//only for debug print
  #include <stdlib.h>
  #include <stdio.h>
#else 
  #ifdef DEBUG1
    #include <stdlib.h>
    #include <stdio.h>
  #endif
#endif

#ifdef USEDMA
  #include "int_dmaswarm.h"
  extern int dmaObject;
#endif

#define DATA_IN_L1 0
#define DATA_IN_L2 2
#define TRANSFER_TO_L1 3
#define TRANSFER_TO_L2 1
#define _32BIT 1

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

#if 0
void	scratch_queue_write(SCRATCH_QUEUE_PRODUCER *queue_p, char *data);
char*	scratch_queue_getToken_write(SCRATCH_QUEUE_PRODUCER *queue_p);
void	scratch_queue_putToken_write(SCRATCH_QUEUE_PRODUCER *queue_p);
void	scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_c, char *buffer);
char*	scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_c);
#endif

void scratch_queue_autoinit_system(void);
SCRATCH_QUEUE_PRODUCER* scratch_queue_autoinit_producer(int core_consumer_id, int queue_id, int n_items, int token_size);
SCRATCH_QUEUE_CONSUMER* scratch_queue_autoinit_consumer(int queue_id, int allocate_local_buffer);

extern SCRATCH_SEMAPHORE sem_mutex;
extern SCRATCH_SEMAPHORE scratch_mutex;
extern unsigned int* current_sem_prod_pointer;
extern unsigned int* current_scratch_pointer;
extern unsigned int* base_sem_cons_pointer;

//write on queue specified data; this function has a suspensive semantic
inline void scratch_queue_write(register SCRATCH_QUEUE_PRODUCER *queue_p,register  char *data){

  #ifdef USEINTERRUPT
    ss_sem_wait(queue_p->semaphore_left);
  #else
    while(scratch_sem_wait(queue_p->semaphore_left)==0);
  #endif  

  /*write message on queue*/
  #ifndef USEDMA
//    memcpy(&queue_p->fifo_p[queue_p->index_write*queue_p->message_size],data,queue_p->message_size);
  int offset=queue_p->index_write*queue_p->message_size;
  for(int i=0; i<queue_p->message_size; i++) queue_p->fifo_p[offset+i]=data[i];
  #else
    //REMEBER dmaObject=int_newobject(_32BIT,0,_32BIT,_32BIT,SCRATCH_BASE,SCRATCH_BASE,DATA_IN_L1,false);
    rtems_interrupt_level level;
    rtems_interrupt_disable(level);//this interruption must block interruption & dispatching!
    int_newnrow((queue_p->message_size)/4,dmaObject);
    int_newl1add((unsigned int)&queue_p->fifo_p[queue_p->index_write*queue_p->message_size],dmaObject);
    int_newl2add((unsigned int)data,dmaObject);
    #ifdef DEBUGSWIDMA
      SHOW_DEBUG("DMA TRANSFER, from, to, lines (32-bit)");
      SHOW_DEBUG_INT(data);
      SHOW_DEBUG_INT(&queue_p->fifo_p[queue_p->index_write*queue_p->message_size]);
      SHOW_DEBUG_INT(queue_p->message_size/4);			
    #endif
    int_newstate1(TRANSFER_TO_L1,dmaObject);
    int_wait(dmaObject);
    #ifdef DEBUGSWIDMA
      SHOW_DEBUG("DMA TRANSFER OK");
    #endif
    rtems_interrupt_enable(level);
  #endif
  /* circular queue */
  ++queue_p->index_write;
  if (queue_p->index_write == queue_p->n_items) queue_p->index_write=0; 

  #ifdef USEINTERRUPT
    ss_sem_signal(queue_p->semaphore_used);
  #else
    scratch_sem_signal(queue_p->semaphore_used);
  #endif
  
}

//the next two function permit to obtain a buffer in the queue and then "commit" it
//this is useful if queue are really allocated on scratch because working in a scratch buffer don't generate bus traffic

//this function permit to obtain the address of the first queue free buffer
//this function has suspensive semantic
inline char* scratch_queue_getToken_write(register SCRATCH_QUEUE_PRODUCER *queue_p){

  #ifdef USEINTERRUPT
    ss_sem_wait(queue_p->semaphore_left);
  #else
    while(scratch_sem_wait(queue_p->semaphore_left)==0);
  #endif 

  return &(queue_p->fifo_p[queue_p->index_write*queue_p->message_size]);
}

//this function "commit" the last "getToken" buffer in the queue
//must be used only sequentially after scratch_queue_getToken_write
inline void scratch_queue_putToken_write(register SCRATCH_QUEUE_PRODUCER *queue_p){
  /* circular queue */
  ++queue_p->index_write;
  if (queue_p->index_write == queue_p->n_items) queue_p->index_write=0; 

  #ifdef USEINTERRUPT
    ss_sem_signal(queue_p->semaphore_used);
  #else
    scratch_sem_signal(queue_p->semaphore_used);
  #endif
}

//read from the queue and write into a buffer; this function has suspensive semantic
inline void scratch_queue_read(register SCRATCH_QUEUE_CONSUMER *queue_p,register char *buffer){
 
  #ifdef USEINTERRUPT
    ss_sem_wait(queue_p->semaphore_used);
  #else
    while(scratch_sem_wait(queue_p->semaphore_used)==0);
  #endif 
   
  /*read message from queue*/
  #ifndef USEDMA
    //memcpy(buffer,&queue_p->fifo_p[queue_p->index_read*queue_p->message_size],queue_p->message_size);
    int offset=queue_p->index_read*queue_p->message_size;
    for(int i=0; i<queue_p->message_size; i++) buffer[i]=queue_p->fifo_p[offset+i];
  #else
    //REMEBER dmaObject=int_newobject(_32BIT,0,_32BIT,_32BIT,SCRATCH_BASE,SCRATCH_BASE,DATA_IN_L1,false);
    rtems_interrupt_level level;
    rtems_interrupt_disable(level);//see above for information
    #ifdef DEBUGSWIDMA
      SHOW_DEBUG("DMA TRANSFER, from, to, lines (32-bit)");
      SHOW_DEBUG_INT(&queue_p->fifo_p[queue_p->index_read*queue_p->message_size]);
      SHOW_DEBUG_INT(buffer);
      SHOW_DEBUG_INT(queue_p->message_size/4);			
    #endif      
    int_newnrow((queue_p->message_size)/4,dmaObject);
    int_newl1add((unsigned int)&queue_p->fifo_p[queue_p->index_read*queue_p->message_size],dmaObject);
    int_newl2add((unsigned int)buffer,dmaObject);
    int_newstate1(TRANSFER_TO_L2,dmaObject);
    int_wait(dmaObject);
    #ifdef DEBUGSWIDMA
      SHOW_DEBUG("DMA TRANSFER OK");
    #endif      
    rtems_interrupt_enable(level);
  #endif
  /*circular queue */
  ++queue_p->index_read;
  if (queue_p->index_read == queue_p->n_items) queue_p->index_read=0; // circular buffer

  #ifdef USEINTERRUPT
    ss_sem_signal(queue_p->semaphore_left);
  #else
    scratch_sem_signal(queue_p->semaphore_left);
  #endif   
  
}

//read from the queue and write into the consumer local buffer, returned as result; this function has suspensive semantic
inline char* scratch_queue_read(register SCRATCH_QUEUE_CONSUMER *queue_p){
  #ifdef DEBUGSWI
    SHOW_DEBUG_INT(queue_p->local_buffer);
    SHOW_DEBUG_INT(&queue_p->fifo_p[queue_p->index_read*queue_p->message_size]);
    SHOW_DEBUG_INT(queue_p->message_size);
  #endif
  scratch_queue_read(queue_p,queue_p->local_buffer);
  return queue_p->local_buffer;
}
#endif
