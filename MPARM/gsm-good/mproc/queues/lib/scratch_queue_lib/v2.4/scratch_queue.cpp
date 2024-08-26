/*
* scratch_queue: queue implemented on suspensive semaphore
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 2.2
* Uses ss_semaphore library
* 
* Original code by  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*/

/*
* Version History & Bug Tracking
* v 2.4 - still working on dma, unified queue read code! (TODO: consider inlining of local_buffered one)
* V 2.3 - Single file version of library, still  working on dma
* V 2.2 - Included dma support for memory transfers
* V 2.1 - General fixing & refining + modified for appsupport including
* V 2.0 - semaphore init bug: semaphore init raised a fake interrupt... cleaning by removing init code:
*	  Hardare itself is initialized at 0 with interrupt flag false.
* v 2.0 - Introduced different header for consumer e producer side of the queue:
* 	  consumer side now contains a local_buffer side address;
*	  read from the queue can be performed copying data to a certain buffer void scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_p, char *buffer)
*         or copying data directly to the local one char* scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_p)
*	  The second method can be used only if the local buffer is really allocated!
*/

// How system works and what the hell it does?
// This library implement queue initilization system
// The initialization system permit concurrent initialization of different queues on memory:
// one mutex (non suspensive) grant access to the first free memory address pointer where queue structure and buffer must be allocated
// one mutex (non suspensive) grant access to the first free producer semaphore pointer where producer must take his semaphore
// for now initialization maps:
// the mutex on the first two semaphore (non suspensive hardware)
// queue's semaphores straight from third semaphore
// first free producer semaphore pointer on address 0 of scratch memory
// first free memory address pointer on address 4 of scratch memory
// consumer semaphore base pointer on address 8 of scratch memory
// suspensive sempahore pad's straight from address C of scratch memory

// What does this library do?
// This library implements utility methods for working on queues
// For queues initialization see multi_scratch_queue library

#include "scratch_queue.h"
#include "scratch_semaphore.h"

#include "config.h" //scratch base + queue base + MPARM configuration parameters
#include <bsp.h> //get_id() redefined on RTEMS + SHOW_DEBUG macros
#include <string.h> //for memcpy function

//#define DEBUG0 //prints on serial ports initialization procedure results
//#define DEBUG1 //prints on serial ports producer/consumer procedure results
//#define DEBUGSWI //prints via swi debug informations during work
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
  int dmaObject;
#endif

#define DATA_IN_L1 0
#define DATA_IN_L2 2
#define TRANSFER_TO_L1 3
#define TRANSFER_TO_L2 1
#define _32BIT 1

SCRATCH_SEMAPHORE sem_mutex;
SCRATCH_SEMAPHORE scratch_mutex;
unsigned int* current_sem_prod_pointer;
unsigned int* current_scratch_pointer;
unsigned int* base_sem_cons_pointer;

void scratch_queue_autoinit_system(void){

  unsigned int scratch_base_address;
  unsigned int sem_base_address;
 
  //initialize base address on start of scratch and queue spaces
  scratch_base_address=(SCRATCH_BASE + (CORESLAVE_SPACING*(get_id()-1)));
  sem_base_address=(QUEUE_BASE + (CORESLAVE_SPACING*(get_id()-1)));

  #ifdef DEBUG0
    printf("id: %x\n",get_id()-1);
    printf("scratch base: %x\n", scratch_base_address);
    printf("sem base: %x\n", sem_base_address);
  #endif

  //initalize mutexes pointers and mutexes values (1)
  sem_mutex=(SCRATCH_SEMAPHORE)sem_base_address;
  scratch_sem_init(sem_mutex,1);
  sem_base_address+=sizeof(SCRATCH_SEMAPHORE);
 
  scratch_mutex=(SCRATCH_SEMAPHORE)sem_base_address;
  scratch_sem_init(scratch_mutex,1);
  sem_base_address+=sizeof(SCRATCH_SEMAPHORE);
 
  //initialize the other pointers pointers (free memory, free producer sempahore, consumer semaphore base)
  //values will be initilized later...
  current_sem_prod_pointer=(unsigned int*)scratch_base_address;
  scratch_base_address+=sizeof(int);
  current_scratch_pointer=(unsigned int*)scratch_base_address;
  scratch_base_address+=sizeof(int);
  base_sem_cons_pointer=(unsigned int*)scratch_base_address;
  scratch_base_address+=sizeof(int);
 
  //initilize suspensive semphore system
  ss_sem_system_init(sem_base_address,scratch_base_address,2*MAXQUEUE);
 
  //initialize other pointers values
  *current_sem_prod_pointer=sem_base_address;//primo semaforo libero
  *base_sem_cons_pointer=sem_base_address+MAXQUEUE*sizeof(SS_SEMAPHORE); 
  *current_scratch_pointer=scratch_base_address+2*MAXQUEUE*sizeof(int); //after suspensive semaphore pad
  
  #ifdef USEDMA
    //initialize dma
    dmaObject=int_newobject(_32BIT,0,_32BIT,_32BIT,SCRATCH_BASE,SCRATCH_BASE,DATA_IN_L1,false);
    //this initialization provides a "fake object" used from the queues to use dma (waiting in polling)
  #endif

  #ifdef DEBUG0
    printf("mutex semafori prod: %x\n", sem_mutex);
    printf("mutex scratch: %x\n", scratch_mutex);
    printf("scratch pointer address: %x\n", current_scratch_pointer);
    printf("scratch pointer value: %x\n", *current_scratch_pointer);
    printf("sem prod pointer address: %x\n", current_sem_prod_pointer);
    printf("sem prod pointer value: %x\n", *current_sem_prod_pointer);
    printf("base sem cons address: %x\n", base_sem_cons_pointer);
    printf("base sem cons value: %x\n", *base_sem_cons_pointer);
  #endif
}

//initialize producer side of queue_id queue to core_consumer_id core
//return the address of the queue header allocated on local scratch
SCRATCH_QUEUE_PRODUCER* scratch_queue_autoinit_producer(int core_consumer_id, int queue_id, int n_items, int token_size){
  unsigned int sem_prod_address;
  unsigned int sem_cons_address;
  unsigned int scratch_area_address;

  //active wait for semaphore mutex
  while((scratch_sem_wait(sem_mutex))==0);
  sem_prod_address=*current_sem_prod_pointer;
  *current_sem_prod_pointer+=sizeof(SS_SEMAPHORE);
  //exit from critical section
  scratch_sem_signal(sem_mutex);

  //active wait for scratch mutex
  while((scratch_sem_wait(scratch_mutex))==0);
  scratch_area_address=*current_scratch_pointer;
  *current_scratch_pointer+=sizeof(SCRATCH_QUEUE_PRODUCER)+n_items*token_size;
  //exit from critical section
  scratch_sem_signal(scratch_mutex);

  //retrieve consumer semaphore address on remote core
  sem_cons_address=QUEUE_BASE+CORESLAVE_SPACING*(core_consumer_id-1)+(2+MAXQUEUE+queue_id)*sizeof(SS_SEMAPHORE); //first two are mutex!
 
  scratch_queue_init_producer(	(SCRATCH_QUEUE_PRODUCER*)scratch_area_address,
 				(char*)(scratch_area_address+sizeof(SCRATCH_QUEUE_PRODUCER)),
				n_items,
				token_size,
				(SS_SEMAPHORE)(unsigned int*)sem_prod_address,
				(SS_SEMAPHORE)(unsigned int*)sem_cons_address);


  //write on remote semaphore address of the queue header, this could resume consumer initilization
  ss_sem_init((SS_SEMAPHORE)sem_cons_address,scratch_area_address);

  #ifdef DEBUGSWI
    SHOW_DEBUG("producer init, written on semaphore");
    SHOW_DEBUG_INT(scratch_area_address);
  #endif
 
  #ifdef DEBUG1
    printf("producer, queue %x node %x\n",queue_id,core_consumer_id);
    printf("address fifo_p %x, address sem_left=%x, address sem_ used=%x\n",
 			((SCRATCH_QUEUE_PRODUCER*)scratch_area_address)->fifo_p,
			((SCRATCH_QUEUE_PRODUCER*)scratch_area_address)->semaphore_left,
			((SCRATCH_QUEUE_PRODUCER*)scratch_area_address)->semaphore_used); 
  #endif
  
  #ifdef DEBUGSWI
    SHOW_DEBUG("producer side queue init");
    SHOW_DEBUG_INT(((SCRATCH_QUEUE_PRODUCER*)scratch_area_address)->fifo_p);
    SHOW_DEBUG_INT(((SCRATCH_QUEUE_PRODUCER*)scratch_area_address)->semaphore_left);
    SHOW_DEBUG_INT(((SCRATCH_QUEUE_PRODUCER*)scratch_area_address)->semaphore_used);        
  #endif
   
  return (SCRATCH_QUEUE_PRODUCER*)scratch_area_address;
}

SCRATCH_QUEUE_CONSUMER* scratch_queue_autoinit_consumer(int queue_id, int allocate_local_buffer){

  SCRATCH_SEMAPHORE sem_init;
  SCRATCH_QUEUE_PRODUCER  *queue_prod;
  SCRATCH_QUEUE_CONSUMER  *queue_cons;
  unsigned int scratch_area_address;
  
  sem_init=(SS_SEMAPHORE)(QUEUE_BASE+CORESLAVE_SPACING*(get_id()-1)+(2+MAXQUEUE+queue_id)*sizeof(SCRATCH_SEMAPHORE));
  queue_prod=(SCRATCH_QUEUE_PRODUCER*)ss_sem_wait_read(sem_init);

  #ifdef DEBUGSWI
    SHOW_DEBUG("consumer init, read on semaphore");
    SHOW_DEBUG_INT(queue_prod);
  #endif

  //active wait for scratch mutex
  while((scratch_sem_wait(scratch_mutex))==0);
  scratch_area_address=*current_scratch_pointer;
  *current_scratch_pointer+=sizeof(SCRATCH_QUEUE_CONSUMER);
  //exit from critical section
  scratch_sem_signal(scratch_mutex);

  queue_cons=(SCRATCH_QUEUE_CONSUMER*)scratch_area_address;
  
  scratch_queue_init_consumer(	queue_cons,
  				queue_prod->fifo_p,
				NULL,
				queue_prod->n_items,
				queue_prod->message_size,
				queue_prod->semaphore_left,
				queue_prod->semaphore_used); 

  ss_sem_init(queue_cons->semaphore_used, 0); //set to 0 because now contains queue header address
  ss_sem_init(queue_cons->semaphore_left,queue_cons->n_items);
  
  if(allocate_local_buffer) {
    //active wait for scratch mutex
    while((scratch_sem_wait(scratch_mutex))==0);
    scratch_area_address=*current_scratch_pointer;
    *current_scratch_pointer+=queue_cons->message_size;
    //exit from critical section
    scratch_sem_signal(scratch_mutex);
    queue_cons->local_buffer=(char*)scratch_area_address;
  }
  
  #ifdef DEBUG1
    printf("consumer, coda %x\n",queue_id);
    printf("address fifo_p %x, address sem_left=%x, address sem_ used=%x\n",
 			queue_cons->fifo_p,
			queue_cons->semaphore_left,
			queue_cons->semaphore_used);
    if(allocate_local_buffer) printf("local_buffer=%x\n",queue_cons->local_buffer);
  #endif
  
  #ifdef DEBUGSWI
    SHOW_DEBUG("consumer side queue init");
    SHOW_DEBUG_INT(queue_cons->fifo_p);
    SHOW_DEBUG_INT(queue_cons->semaphore_left);
    SHOW_DEBUG_INT(queue_cons->semaphore_used); 
    if(allocate_local_buffer) SHOW_DEBUG_INT(queue_cons->local_buffer); 
  #endif
  
  return queue_cons;
}

//initialize a queue producer side
void scratch_queue_init_producer(SCRATCH_QUEUE_PRODUCER *queue_p, char *m_fifo, int n_items, int token_size,SS_SEMAPHORE producer_sem,SS_SEMAPHORE consumer_sem){
  queue_p->index_write = 0;
  queue_p->message_size = token_size;
  queue_p->n_items = n_items;
  queue_p->fifo_p = m_fifo;
  queue_p->semaphore_left = producer_sem;
  queue_p->semaphore_used = consumer_sem;
//  ss_sem_init(queue_p->semaphore_left, 0); init bug, see above for details
//  ss_sem_init(queue_p->semaphore_used, 0); init bug, see above for details
}

//initialize a queue consumer side
void scratch_queue_init_consumer(SCRATCH_QUEUE_CONSUMER *queue_p, char *m_fifo, char* local_buffer, int n_items, int token_size,SS_SEMAPHORE producer_sem,SS_SEMAPHORE consumer_sem){
  queue_p->index_read = 0;
  queue_p->message_size = token_size;
  queue_p->n_items = n_items;
  queue_p->fifo_p = m_fifo;
  queue_p->local_buffer = local_buffer;
  queue_p->semaphore_left = producer_sem;
  queue_p->semaphore_used = consumer_sem;
}

//write on queue specified data; this function has a suspensive semantic
void scratch_queue_write(SCRATCH_QUEUE_PRODUCER *queue_p, char *data){
  ss_sem_wait(queue_p->semaphore_left);
  /*write message on queue*/
  #ifndef USEDMA
    memcpy(&queue_p->fifo_p[queue_p->index_write*queue_p->message_size],data,queue_p->message_size);
  #else
    //REMEBER dmaObject=int_newobject(_32BIT,0,_32BIT,_32BIT,SCRATCH_BASE,SCRATCH_BASE,DATA_IN_L1,false);
    if(((queue_p->message_size)%4)==0){
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
    }
    else memcpy(&queue_p->fifo_p[queue_p->index_write*queue_p->message_size],data,queue_p->message_size);
  #endif
  /* circular queue */
  ++queue_p->index_write;
  if (queue_p->index_write == queue_p->n_items) queue_p->index_write=0; 
  ss_sem_signal(queue_p->semaphore_used);
}

//the next two function permit to obtain a buffer in the queue and then "commit" it
//this is useful if queue are really allocated on scratch because working in a scratch buffer don't generate bus traffic

//this function permit to obtain the address of the first queue free buffer
//this function has suspensive semantic
char* scratch_queue_getToken_write(SCRATCH_QUEUE_PRODUCER *queue_p){
  ss_sem_wait(queue_p->semaphore_left);
  return &(queue_p->fifo_p[queue_p->index_write*queue_p->message_size]);
}

//this function "commit" the last "getToken" buffer in the queue
//must be used only sequentially after scratch_queue_getToken_write
void scratch_queue_putToken_write(SCRATCH_QUEUE_PRODUCER *queue_p){
  /* circular queue */
  ++queue_p->index_write;
  if (queue_p->index_write == queue_p->n_items) queue_p->index_write=0; 
  ss_sem_signal(queue_p->semaphore_used); 
}

//read from the queue and write into a buffer; this function has suspensive semantic
void scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_p, char *buffer){
  ss_sem_wait(queue_p->semaphore_used);
  /*read message from queue*/
  #ifndef USEDMA
    memcpy(buffer,&queue_p->fifo_p[queue_p->index_read*queue_p->message_size],queue_p->message_size);
  #else
    //REMEBER dmaObject=int_newobject(_32BIT,0,_32BIT,_32BIT,SCRATCH_BASE,SCRATCH_BASE,DATA_IN_L1,false);
    if(((queue_p->message_size)%4)==0){
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
    }
    else memcpy(buffer,&queue_p->fifo_p[queue_p->index_read*queue_p->message_size],queue_p->message_size);
  #endif
  /*circular queue */
  ++queue_p->index_read;
  if (queue_p->index_read == queue_p->n_items) queue_p->index_read=0; // circular buffer
  ss_sem_signal(queue_p->semaphore_left);
}

//read from the queue and write into the consumer local buffer, returned as result; this function has suspensive semantic
char* scratch_queue_read(SCRATCH_QUEUE_CONSUMER *queue_p){
  #ifdef DEBUGSWI
    SHOW_DEBUG_INT(queue_p->local_buffer);
    SHOW_DEBUG_INT(&queue_p->fifo_p[queue_p->index_read*queue_p->message_size]);
    SHOW_DEBUG_INT(queue_p->message_size);
  #endif
  scratch_queue_read(queue_p,queue_p->local_buffer);
  return queue_p->local_buffer;
}
