/*
* scratch_queue: queue implemented on suspensive semaphore
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 2.91
* Uses ss_semaphore library
* 
* Original code by  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*/

/*
* Version History & Bug Tracking
* v 2.91 - Memory transfer bug eliminated
* v 2.9 - Memory tranfser code optimized for 32-bit transfer
* v 2.8 - Eliminated memcpy function
* v 2.7 - Aligned to 4 bytes size of the item for ARM read cicle compatibility
* v 2.6 - Optimized adding register type to some variable and inline to some crucial functions
* v 2.5 - unified suspensive and active waiting libraries, switch mode by defining USEINTERRUPT macro
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

#ifdef USEDMA
  int dmaObject;
#endif

SCRATCH_SEMAPHORE sem_mutex;
SCRATCH_SEMAPHORE scratch_mutex;
unsigned int* current_sem_prod_pointer;
unsigned int* current_scratch_pointer;
unsigned int* base_sem_cons_pointer;

void scratch_queue_autoinit_system(void){

  unsigned int scratch_base_address;
  unsigned int sem_base_address;
 
  //initialize base address on start of scratch and queue spaces
  scratch_base_address=(CORESLAVE_BASE + (CORESLAVE_SPACING*(get_id()-1)));
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

  #ifdef USEINTERRUPT 
    //initilize suspensive semphore system
    ss_sem_system_init(sem_base_address,scratch_base_address,2*MAXQUEUE);
  #endif
 
  //initialize other pointers values
  *current_sem_prod_pointer=sem_base_address;//first free semaphore
  *base_sem_cons_pointer=sem_base_address+MAXQUEUE*QUEUE_SEMAPHORE_SIZE;
  #ifdef USEINTERRUPT
    *current_scratch_pointer=scratch_base_address+2*MAXQUEUE*sizeof(int); //after suspensive semaphore pad
  #else
    *current_scratch_pointer=scratch_base_address; //without pad
  #endif
  
  #ifdef USEDMA
    //initialize dma
    dmaObject=int_newobject(_32BIT,0,_32BIT,_32BIT,CORESLAVE_BASE,CORESLAVE_BASE,DATA_IN_L1,false);
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

//initialize producer side of queue_id queue to core_consumer_id core. queue_id in [0..MAXQUEUE-1]. core_consumer_id in [1..gettotcores]
//return the address of the queue header allocated on local scratch
SCRATCH_QUEUE_PRODUCER* scratch_queue_autoinit_producer(int core_consumer_id, int queue_id, int n_items, int token_size){
  unsigned int sem_prod_address;
  unsigned int sem_cons_address;
  unsigned int scratch_area_address;
  
  //to align fifo address with 4
  if(token_size%4) token_size=token_size+(4-(token_size%4));

  //active wait for semaphore mutex
  while((scratch_sem_wait(sem_mutex))==0);
  sem_prod_address=*current_sem_prod_pointer;
  *current_sem_prod_pointer+=QUEUE_SEMAPHORE_SIZE;
  //exit from critical section
  scratch_sem_signal(sem_mutex);

  //active wait for scratch mutex
  while((scratch_sem_wait(scratch_mutex))==0);
  scratch_area_address=*current_scratch_pointer;
  *current_scratch_pointer+=sizeof(SCRATCH_QUEUE_PRODUCER)+n_items*token_size;
  //exit from critical section
  scratch_sem_signal(scratch_mutex);

  //retrieve consumer semaphore address on remote core
  sem_cons_address=QUEUE_BASE+CORESLAVE_SPACING*(core_consumer_id-1)+(2+MAXQUEUE+queue_id)*QUEUE_SEMAPHORE_SIZE; //first two are mutex!

  scratch_queue_init_producer(	(SCRATCH_QUEUE_PRODUCER*)scratch_area_address,
 				(char*)(scratch_area_address+sizeof(SCRATCH_QUEUE_PRODUCER)),
				n_items,
				token_size,
				(QUEUE_SEMAPHORE)(unsigned int*)sem_prod_address,
				(QUEUE_SEMAPHORE)(unsigned int*)sem_cons_address);

  //write on remote semaphore address of the queue header, this could resume consumer initilization
  #ifdef USEINTERRUPT
    ss_sem_init((SS_SEMAPHORE)sem_cons_address,scratch_area_address);
  #else
    scratch_sem_init((SCRATCH_SEMAPHORE)sem_cons_address,scratch_area_address);
  #endif

  #ifdef DEBUGSWI
    SHOW_DEBUG("Producer init, written on semaphore");
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
    SHOW_DEBUG("Producer side queue init");
    SHOW_DEBUG_INT(((SCRATCH_QUEUE_PRODUCER*)scratch_area_address)->fifo_p);
    SHOW_DEBUG_INT(((SCRATCH_QUEUE_PRODUCER*)scratch_area_address)->semaphore_left);
    SHOW_DEBUG_INT(((SCRATCH_QUEUE_PRODUCER*)scratch_area_address)->semaphore_used);        
  #endif
   
  return (SCRATCH_QUEUE_PRODUCER*)scratch_area_address;
}

SCRATCH_QUEUE_CONSUMER* scratch_queue_autoinit_consumer(int queue_id, bool allocate_local_buffer){

  QUEUE_SEMAPHORE sem_init;
  SCRATCH_QUEUE_PRODUCER  *queue_prod;
  SCRATCH_QUEUE_CONSUMER  *queue_cons;
  unsigned int scratch_area_address;
  
  sem_init=(QUEUE_SEMAPHORE)(QUEUE_BASE+CORESLAVE_SPACING*(get_id()-1)+(2+MAXQUEUE+queue_id)*QUEUE_SEMAPHORE_SIZE);
  #ifdef USEINTERRUPT
    queue_prod=(SCRATCH_QUEUE_PRODUCER*)ss_sem_wait_read(sem_init);
  #else
    while((queue_prod=(SCRATCH_QUEUE_PRODUCER*)scratch_sem_wait(sem_init))==0);
  #endif

  #ifdef DEBUGSWI
    SHOW_DEBUG("Consumer init, read on semaphore");
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

  #ifdef USEINTERRUPT
    ss_sem_init(queue_cons->semaphore_used, 0); //set to 0 because now contains queue header address
    ss_sem_init(queue_cons->semaphore_left,queue_cons->n_items);
  #else
    scratch_sem_init(queue_cons->semaphore_used, 0); //set to 0 because now contains queue header address
    scratch_sem_init(queue_cons->semaphore_left,queue_cons->n_items);
  #endif
  
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
    SHOW_DEBUG("Consumer side queue init");
    SHOW_DEBUG_INT(queue_cons->fifo_p);
    SHOW_DEBUG_INT(queue_cons->semaphore_left);
    SHOW_DEBUG_INT(queue_cons->semaphore_used); 
    if(allocate_local_buffer) SHOW_DEBUG_INT(queue_cons->local_buffer); 
  #endif
  
  return queue_cons;
}

//initialize a queue producer side
void scratch_queue_init_producer(SCRATCH_QUEUE_PRODUCER *queue_p, char *m_fifo, int n_items, int token_size,QUEUE_SEMAPHORE producer_sem,QUEUE_SEMAPHORE consumer_sem){
  queue_p->index_write = 0;
  queue_p->message_size = token_size;
  queue_p->n_items = n_items;
  queue_p->fifo_p = m_fifo;
  queue_p->semaphore_left = producer_sem;
  queue_p->semaphore_used = consumer_sem;
//  ss/scratc_sem_init(queue_p->semaphore_left, 0); init bug, see above for details
//  ss/scratch_sem_init(queue_p->semaphore_used, 0); init bug, see above for details
}

//initialize a queue consumer side
void scratch_queue_init_consumer(SCRATCH_QUEUE_CONSUMER *queue_p, char *m_fifo, char* local_buffer, int n_items, int token_size,QUEUE_SEMAPHORE producer_sem,QUEUE_SEMAPHORE consumer_sem){
  queue_p->index_read = 0;
  queue_p->message_size = token_size;
  queue_p->n_items = n_items;
  queue_p->fifo_p = m_fifo;
  queue_p->local_buffer = local_buffer;
  queue_p->semaphore_left = producer_sem;
  queue_p->semaphore_used = consumer_sem;
}

void* getFreeScratchAddress(){
  return (void*)(*current_scratch_pointer);
}
