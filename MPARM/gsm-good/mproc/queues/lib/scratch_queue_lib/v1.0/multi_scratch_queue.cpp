/*
* multi_scratch_queue: initialization of queue implemented on suspensive semaphore
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 1.0
* Uses ss_semaphore library, scratch_queue, scratch_semaphore
* 
* Original code by  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
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

#include "multi_scratch_queue.h"

#include "appsupport.h" //scratch base + queue base + MPARM configuration parameters
#include <bsp.h> //get_id() redefined on RTEMS
#include <string.h> //for memcpy function
#include "scratch_semaphore.h"

//#define DEBUG0 //prints on serial ports initialization procedure results
//#define DEBUG1 //prints on serial ports producer/consumer procedure results
#define DEBUGSWI //prints via swi debug informations

#ifdef DEBUG0//only for debug print
  #include <stdlib.h>
  #include <stdio.h>
#else 
  #ifdef DEBUG1
    #include <stdlib.h>
    #include <stdio.h>
  #endif
#endif

#ifdef DEBUGSWI
  #include "appsupport.h"
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
  *current_scratch_pointer=scratch_base_address+2*MAXQUEUE*sizeof(int); //dopo i pad dei semafori sospensivi!

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
SCRATCH_QUEUE* scratch_queue_autoinit_producer(	int core_consumer_id,int queue_id,int n_items,int token_size){
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
  *current_scratch_pointer+=sizeof(SCRATCH_QUEUE)+n_items*token_size;
  //exit from critical section
  scratch_sem_signal(scratch_mutex);

  //retrieve consumer semaphore address on remote core
  sem_cons_address=QUEUE_BASE+CORESLAVE_SPACING*(core_consumer_id-1)+(2+MAXQUEUE+queue_id)*sizeof(SS_SEMAPHORE); //first two are mutex!
 
  scratch_queue_init_producer(	(SCRATCH_QUEUE*)scratch_area_address,
 				(char*)(scratch_area_address+sizeof(SCRATCH_QUEUE)),
				n_items,
				token_size,
				(SS_SEMAPHORE)(unsigned int*)sem_prod_address,
				(SS_SEMAPHORE)(unsigned int*)sem_cons_address);

  //write on remote semaphore address of the queue header, this could resume consumer initilization
  ss_sem_init((SS_SEMAPHORE)sem_cons_address,scratch_area_address);
 
  #ifdef DEBUG1
    printf("producer, coda %x nodo %x\n",queue_id,core_consumer_id);
    printf("address fifo_p %x, address sem_left=%x, address sem_ used=%x\n",
 			((SCRATCH_QUEUE*)scratch_area_address)->fifo_p,
			((SCRATCH_QUEUE*)scratch_area_address)->semaphore_left,
			((SCRATCH_QUEUE*)scratch_area_address)->semaphore_used); 
  #endif
  
  #ifdef DEBUGSWI
    SHOW_DEBUG("producer side queue init");
    SHOW_DEBUG_INT(((SCRATCH_QUEUE*)scratch_area_address)->fifo_p);
    SHOW_DEBUG_INT(((SCRATCH_QUEUE*)scratch_area_address)->semaphore_left);
    SHOW_DEBUG_INT(((SCRATCH_QUEUE*)scratch_area_address)->semaphore_used);        
  #endif
   
  return (SCRATCH_QUEUE*)scratch_area_address;
}

//writes on queue_cons the header of the queue (retrives it from producer core by the address written on the local sempahore);
void scratch_queue_autoinit_consumer(SCRATCH_QUEUE *queue_cons,int queue_id){

  SCRATCH_SEMAPHORE sem_init;
  unsigned int address_copy;
  sem_init=(SS_SEMAPHORE)(QUEUE_BASE+CORESLAVE_SPACING*(get_id()-1)+(2+MAXQUEUE+queue_id)*sizeof(SCRATCH_SEMAPHORE));

  address_copy=ss_sem_wait_read(sem_init);

  memcpy((char*)queue_cons,(char*)(address_copy),sizeof(SCRATCH_QUEUE));

  ss_sem_init(queue_cons->semaphore_used, 0);
  ss_sem_init(queue_cons->semaphore_left,queue_cons->n_items);
  
  #ifdef DEBUG1
    printf("consumer, coda %x\n",queue_id);
    printf("address fifo_p %x, address sem_left=%x, address sem_ used=%x\n",
 			queue_cons->fifo_p,
			queue_cons->semaphore_left,
			queue_cons->semaphore_used);
  #endif
  #ifdef DEBUGSWI
    SHOW_DEBUG("consumer side queue init");
    SHOW_DEBUG_INT(queue_cons->fifo_p);
    SHOW_DEBUG_INT(queue_cons->semaphore_left);
    SHOW_DEBUG_INT(queue_cons->semaphore_used);        
  #endif
}
