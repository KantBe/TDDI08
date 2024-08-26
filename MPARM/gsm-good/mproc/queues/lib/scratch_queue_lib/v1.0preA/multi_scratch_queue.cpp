#include "multi_scratch_queue.h" // proprio header

#include "appsupport.h" //scratch base + queue base, parametri config MPARM
#include <bsp.h> //per avere get_id() ridefinito su RTEMS: incluso perche' usiamo RTEMS

#include <string.h> //per la memcpy

//#define DEBUG

#ifdef DEBUG
#include <stdlib.h> //solo per le print di debug
#include <stdio.h>  //solo per le print di debug
#endif



SCRATCH_SEMAPHORE sem_mutex;
SCRATCH_SEMAPHORE scratch_mutex;
unsigned int* current_sem_prod_pointer;
unsigned int* current_scratch_pointer;
unsigned int* base_sem_cons_pointer;

/*===========  scratch_queue_autoinit_system  ============================*/
/*
*  Inizializza struttura e valori per l'istanziazione delle code,
*  da invocare una sola volta per core.
*/ 

void scratch_queue_autoinit_system(void){

 unsigned int scratch_base_address;
 unsigned int sem_base_address;

 //inizializzo il puntatore alla base dello scracth e della queue
 scratch_base_address=(SCRATCH_BASE + (CORESLAVE_SPACING*(get_id()-1)));
 sem_base_address=(QUEUE_BASE + (CORESLAVE_SPACING*(get_id()-1)));
#ifdef DEBUG
 printf("id: %x\n",get_id()-1);
 printf("scratch base: %x\n", scratch_base_address);
 printf("sem base: %x\n", sem_base_address);
#endif

 //inizializzo indirizzi e valori dei semafori mutex spostando il p. alla base della queue
 sem_mutex=(SCRATCH_SEMAPHORE)sem_base_address;
 scratch_sem_init(sem_mutex,1);
 sem_base_address+=sizeof(SCRATCH_SEMAPHORE);
 
 scratch_mutex=(SCRATCH_SEMAPHORE)sem_base_address;
 scratch_sem_init(scratch_mutex,1);
 sem_base_address+=sizeof(SCRATCH_SEMAPHORE);
 
 //inizializzo sulla scratch i puntatori ai semafori ed alla scratch
 current_sem_prod_pointer=(unsigned int*)scratch_base_address;
 scratch_base_address+=sizeof(int);
 current_scratch_pointer=(unsigned int*)scratch_base_address;
 scratch_base_address+=sizeof(int);
 base_sem_cons_pointer=(unsigned int*)scratch_base_address;
 scratch_base_address+=sizeof(int);

 *current_sem_prod_pointer=sem_base_address;//primo semaforo libero
 *current_scratch_pointer=scratch_base_address; 
 *base_sem_cons_pointer=sem_base_address+MAXQUEUE*sizeof(SCRATCH_SEMAPHORE);
#ifdef DEBUG
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


/*===========  scratch_queue_autoinit_producer  ============================*/
/*
* Allocates a queue on the producer side guessing all hardware-dependent parameters, then initializes the queue through scratch_queue_init_producer
*/ 


SCRATCH_QUEUE* scratch_queue_autoinit_producer(	int core_consumer_id,int queue_id,int n_items,int token_size){
 unsigned int sem_prod_address;
 unsigned int sem_cons_address;
 unsigned int scratch_area_address;

 //attendo sul mutex per prendere un semaforo produzione;
 while((scratch_sem_wait(sem_mutex))==0);
 sem_prod_address=*current_sem_prod_pointer;
 *current_sem_prod_pointer+=sizeof(SCRATCH_SEMAPHORE);
 //esco dalla sezione critica
 scratch_sem_signal(sem_mutex);

 //attendo sul mutex per prendere un pezzo di scratch
 while((scratch_sem_wait(scratch_mutex))==0);
 scratch_area_address=*current_scratch_pointer;
 *current_scratch_pointer+=sizeof(SCRATCH_QUEUE)+n_items*token_size;
 //esco dalla sezione critica
 scratch_sem_signal(scratch_mutex);

 //recupero semaforo sul core remoto.
 sem_cons_address=QUEUE_BASE+CORESLAVE_SPACING*(core_consumer_id-1)+(2+MAXQUEUE+queue_id)*sizeof(SCRATCH_SEMAPHORE);
 
 scratch_queue_init_producer(	(SCRATCH_QUEUE*)scratch_area_address,
 				(char*)(scratch_area_address+sizeof(SCRATCH_QUEUE)),
				n_items,
				token_size,
				(SCRATCH_SEMAPHORE)(unsigned int*)sem_prod_address,
				(SCRATCH_SEMAPHORE)(unsigned int*)sem_cons_address);

 scratch_sem_init((SCRATCH_SEMAPHORE)sem_cons_address,scratch_area_address);
 
#ifdef DEBUG
 printf("producer, coda %x nodo %x\n",queue_id,core_consumer_id);
 printf("address fifo_p %x, address sem_left=%x, address sem_ used=%x\n",
 			((SCRATCH_QUEUE*)scratch_area_address)->fifo_p,
			((SCRATCH_QUEUE*)scratch_area_address)->semaphore_left,
			((SCRATCH_QUEUE*)scratch_area_address)->semaphore_used); 
#endif
   
 return (SCRATCH_QUEUE*)scratch_area_address;
  
}

/*===========  scratch_queue_autoinit_consumer ============================*/
/*
* Initialize a queue on the consumer side and complete semaphores initializations
* to signal the end of initialization/allocation for that queue.
*/ 

void scratch_queue_autoinit_consumer(SCRATCH_QUEUE *queue_cons,int queue_id){

  SCRATCH_SEMAPHORE sem_init;
  unsigned int address_copy;
  sem_init=(SCRATCH_SEMAPHORE)(QUEUE_BASE+CORESLAVE_SPACING*(get_id()-1)+(2+MAXQUEUE+queue_id)*sizeof(SCRATCH_SEMAPHORE));

  while((address_copy=scratch_sem_wait(sem_init))==0);

  memcpy((char*)queue_cons,(char*)(address_copy),sizeof(SCRATCH_QUEUE));

  scratch_sem_init(queue_cons->semaphore_used, 0);
  scratch_sem_init(queue_cons->semaphore_left,queue_cons->n_items);
  
#ifdef DEBUG
 printf("consumer, coda %x\n",queue_id);
 printf("address fifo_p %x, address sem_left=%x, address sem_ used=%x\n",
 			queue_cons->fifo_p,
			queue_cons->semaphore_left,
			queue_cons->semaphore_used); 
#endif  
}
