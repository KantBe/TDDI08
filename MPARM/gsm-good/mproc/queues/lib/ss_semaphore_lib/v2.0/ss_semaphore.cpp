/*
* ss_semaphore: suspensive scratch semaphore library for MPARM.
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v2.0
*/

/*
* Version History & Bug Tracking
* v 2.0 - BUG in unsigned int ss_sem_wait_read(SS_SEMAPHORE my_semaphore)
* 	  output value was depending on task behaviour (suspension or not)
*	  CORRECTED, I HOPE!!
* v 2.0 - Added unsigned int ss_sem_wait_read(SS_SEMAPHORE my_semaphore) function, see below for further information.
* v 1.1 - interrupts enabling bug on task suspension resolved, I hope...
* v 1.1 - WARNING: enabling DF flag for directive_failed calls will crash the library in interrupt handler routine - RESOLVED!!
* v 1.1 - DF resolved invoking directive_failed_with_level macro
*/

// How system works? (it works? :-)) (maybe it's weather dependent...) (ohter bands play, Manowar kills!)
// Using interrupt-raising hardware semaphore we have build a suspensive semaphore library.
// Every semaphore can hold a task suspended on it, resuming it on the first signal.
// For each semaphore we use a four byte pad (tipically in the scratch memory) to store the task id of the task suspended on the semaphore.

// How Hardware works?
// SystemC file that contain semaphore description is swarm/core/scratch_mem.h
// on a read (wait) operation:
// if data in the semaphore is greater than zero semaphore will be decremented
// else the suspended flag of the semaphore will be raised to true.
// on a write (signal) operation:
// if the data written to the semaphore is zero semaphore value will be incremented
// else semaphore value will be initialized to data-1 (this is used for initialization purpose)
// anyway if flag_suspended is true an interrupt will be raised on the local core and the flag will be set to false

// How the interrupt handler works?
// for every semaphore this routine check if there's a task id written on the relative scratch-associated pad
// if there's a task id then the routine will check the semaphore value (this operation makes an implicit "wait"):
// if the value is greater than zero the task will be resumed and semaphore value lowered by one by the implicit "wait"
// else nothing (because no one called a signal on that semaphore) and the "wait" will set another time the suspended flag to true

// What does ss_sem_wait_read(SS_SEMAPHORE my_semaphore) do ?
// This function, after the normal task suspension, returns the value inside the semaphore.
// This function can introduce the undesired side effect of unnecessary interrupts:
// if there's a task suspended on a semaphore and a signal occour on it the value of the semaphore will be raised to 1
// the interrupt handler testing the value will lower it to 0 and will resume the task
// now if the task called ss_sem_wait_read it will perform a read on the semaphore to retrieve the actual value (0)
// this will set the hardware flag of the semaphore to true
// the first write on it will raise an interrupt without suspended task to resume
// anyway if task is resumed by a init (not by a signal) with a value X greater than 1 no fake interrupt will be raised
// in this case the function will return X-1

#include "ss_semaphore.h"
#include "tmacros.h"
#include <bsp.h>
#include "config.h" //for MPARM hardware parameters

#define INT_NUMBER 0 //zero is the cpu interrupt connected pic intreq

//#define DEBUG0 //prints on serial ports
#define DEBUG1 //print via swi calls
//#define DF //invokes directive failed after rtems calls
//#define ESC //Explicit status check after rtems calls

#ifdef DEBUG0 //only for debug print
#include <stdlib.h>
#include <stdio.h>
#endif

#ifdef DEBUG1 //only for debug print
#include "appsupport.h"
#endif


unsigned int *queue_table, *scratch_table;
int number;

//interrupt handler
void interrupt_handler(){
 #ifdef DEBUG1
   SHOW_DEBUG("INTERRUPT: checking for suspended tasks");
 #endif
 #ifdef DEBUG0
   printf("INTERRUPT: checking for suspended tasks\n");
 #endif
 rtems_interrupt_level level;
 rtems_interrupt_disable(level);
 for(int i=0;i<number;i++) if(scratch_table[i]!=0){
 	int value=queue_table[i];
	if(value>0) {
		rtems_status_code status;
		unsigned int id=scratch_table[i];
		scratch_table[i]=0;
		#ifdef DEBUG1
		 SHOW_DEBUG("INTERRUPT: resuming task:");
		 SHOW_DEBUG_INT(id);
		#endif
		#ifdef DEBUG0
		  printf("INTERRUPT: resuming task %x at %x\n",id,&scratch_table[i]);
		#endif
		status=rtems_task_resume(id);
		#ifdef ESC
		  SHOW_DEBUG_INT(status);
		  if(status==RTEMS_SUCCESSFUL) SHOW_DEBUG("RTEMS_SUCCESSFUL");		  
		  if(status==RTEMS_INVALID_ID) SHOW_DEBUG("RTEMS_INVALID_ID");
		  if(status==RTEMS_INCORRECT_STATE) SHOW_DEBUG("RTEMS_INCORRECT_STATE");
		#endif
		#ifdef DF
//		  directive_failed( status, "rtems_task_resume" );// removed for df WARNING, resolved, see below
  		  fatal_directive_status_with_level(status, RTEMS_SUCCESSFUL,"rtems_task_resume",1);
		#endif
	}
 }
 #ifdef DEBUG0
   printf("INTERRUPT: completed\n");
 #endif
 #ifdef DEBUG1
   SHOW_DEBUG("INTERRUPT: completed");
 #endif
 rtems_interrupt_enable(level);
}

//initialize ss_semaphore system building library data structures
int ss_sem_system_init(unsigned int queue_address,unsigned int scratch_address, int num){
 queue_table=(unsigned int*)queue_address;
 scratch_table=(unsigned int*)scratch_address;
 number=num;
 
 #ifdef DEBUG0
   printf("suspensive semaphore base: %x\n", queue_table);
   printf("suspensive semaphore pad base: %x\n", scratch_table);
   printf("suspensive semaphore number: %x\n", num);
 #endif
 
 for(int i=0;i<number;i++) scratch_table[i]=0;
// for(int i=0;i<number;i++) ss_sem_init((SS_SEMAPHORE)queue_table[i],0);
//clean semaphores, it's better to leave this task to applications

 static rtems_irq_connect_data dma_int_data = { int22, interrupt_handler, son, soff, sison, 0, 0};
 if(BSP_install_rtems_irq_handler(&dma_int_data)==0) return 0;
 dma_int_data.on(&dma_int_data);
 return 1;
}

void ss_sem_wait(SS_SEMAPHORE my_semaphore){
 int value;
 rtems_interrupt_level level;
 rtems_id my_id;

 rtems_interrupt_disable(level);
 #ifdef DEBUG0
   printf("ss_sem_wait: %x\n", my_semaphore);
 #endif
 value=(*my_semaphore);

 if(value<=0){
 	rtems_status_code status;
	rtems_task_ident(RTEMS_SELF,get_id(),&my_id);
	//we must write on scratch associated pad the task id
	unsigned int displacement=((unsigned int)my_semaphore-(unsigned int)queue_table)/4;
	#ifdef DEBUG0
	  printf("blocking wait: writing task id %x at %x\n",my_id,&scratch_table[displacement]);
	#endif
	#ifdef DEBUG1
	  SHOW_DEBUG("blocking wait: writing task id:");
	  SHOW_DEBUG_INT(my_id);
	#endif
	scratch_table[displacement]=my_id;
	status=rtems_task_suspend(my_id);//interrupts enabling occur automatically when task is suspended
	rtems_interrupt_enable(level);//once task is resumed, i think, interrupts return disabled and we have to manually enable them
	#ifdef ESC
	  SHOW_DEBUG_INT(status);
	  if(status==RTEMS_SUCCESSFUL) SHOW_DEBUG("RTEMS_SUCCESSFUL");
	  if(status==RTEMS_INVALID_ID) SHOW_DEBUG("RTEMS_INVALID_ID");
	  if(status==RTEMS_ALREADY_SUSPENDED) SHOW_DEBUG("RTEMS_ALREADY_SUSPENDED");
	#endif
	#ifdef DF
	  directive_failed(status,"rtems_task_suspended");
	#endif
	#ifdef DEBUG0
	  printf("resumed task %x\n",my_id);
	#endif
	#ifdef DEBUG1
	  SHOW_DEBUG("Task resumed:");
	  SHOW_DEBUG_INT(my_id);
	#endif
 }
 else{
  rtems_interrupt_enable(level);
 }
}

unsigned int ss_sem_wait_read(SS_SEMAPHORE my_semaphore){
  unsigned int value;
  rtems_interrupt_level level;
  rtems_id my_id;

  rtems_interrupt_disable(level);
  #ifdef DEBUG0
    printf("ss_sem_wait_read: %x\n", my_semaphore);
  #endif
  value=(*my_semaphore);

  if(value<=0){
    rtems_status_code status;
    rtems_task_ident(RTEMS_SELF,get_id(),&my_id);
    //we must write on scratch associated pad the task id
    unsigned int displacement=((unsigned int)my_semaphore-(unsigned int)queue_table)/4;
    #ifdef DEBUG0
      printf("blocking wait: writing task id %x at %x\n",my_id,&scratch_table[displacement]);
    #endif
    #ifdef DEBUG1
      SHOW_DEBUG("blocking wait: writing task id:");
      SHOW_DEBUG_INT(my_id);
    #endif
    scratch_table[displacement]=my_id;
    status=rtems_task_suspend(my_id);//interrupts enabling occur automatically when task is suspended
    rtems_interrupt_enable(level);//once task is resumed, i think, interrupts return disabled and we have to manually enable them
    #ifdef ESC
      SHOW_DEBUG_INT(status);
      if(status==RTEMS_SUCCESSFUL) SHOW_DEBUG("RTEMS_SUCCESSFUL");
      if(status==RTEMS_INVALID_ID) SHOW_DEBUG("RTEMS_INVALID_ID");
      if(status==RTEMS_ALREADY_SUSPENDED) SHOW_DEBUG("RTEMS_ALREADY_SUSPENDED");
    #endif
    #ifdef DF
      directive_failed(status,"rtems_task_suspended");
    #endif
    #ifdef DEBUG0
      printf("resumed task %x\n",my_id);
    #endif
    #ifdef DEBUG1
      SHOW_DEBUG("Task resumed:");
      SHOW_DEBUG_INT(my_id);
    #endif
    value=(*my_semaphore)+1;
  }
  else rtems_interrupt_enable(level);
  return value;
}

void ss_sem_signal(SS_SEMAPHORE my_semaphore){
 #ifdef DEBUG0
   printf("ss_sem_signal: %x\n", my_semaphore);
 #endif
 *my_semaphore = 0;
}

void ss_sem_init(SS_SEMAPHORE my_semaphore,int value){
 #ifdef DEBUG0
   printf("ss_sem_init: %x value %x\n", my_semaphore,value);
 #endif
 *my_semaphore = value+1;
}

//other bands play, Manowar kills!
