#define IRQ_SHARED_PIN EXT_INT

#define MAX_SUPPORTED_TASK 5

//base pointer to the interrupt device
volatile unsigned int* int_base=(unsigned int*)INTERRUPT_BASE;
//base pointer to the semaphore device
volatile unsigned int* sem_base=(unsigned int*)SEMAPHORE_BASE;

//id of the suspended tasks
unsigned int sus_table[MAX_SUPPORTED_TASK];
//number of suspended tasks
unsigned int number=0;

inline void send_int(unsigned int proc)
{
 #ifdef DEBUGSWI
  SHOW_DEBUG("Send_int");
  SHOW_DEBUG_INT(proc);
 #endif
 int_base[proc]=1;
};

inline void task_sus_on_sem(unsigned int sem)
{ 
  int value;
  
  //disable interrupt
  register rtems_interrupt_level level;
  #ifdef DEBUGSWII
    SHOW_DEBUG("WAIT: disabling interrupts");
  #endif
    rtems_interrupt_disable(level);
  #ifdef DEBUGSWI
    SHOW_DEBUG("ss_sem_wait");
    SHOW_DEBUG_INT(my_semaphore);
  #endif
  
 while(1)
 {
  value=sem_base[sem];
  
  //#ifdef DEBUGSWII
  SHOW_DEBUG("Sem_value:,sem:");
  SHOW_DEBUG_INT(value);
  SHOW_DEBUG_INT(&sem_base[sem]);
  //#endif

  if(value==1){
    
    register rtems_id my_id;
    register rtems_status_code status;
    rtems_task_ident(RTEMS_SELF,get_id(),&my_id);

    //#ifdef DEBUGSWI
      SHOW_DEBUG("blocking wait: writing task id:");
      SHOW_DEBUG_INT(my_id);
      SHOW_DEBUG_INT(&sem_base[sem]);
    //#endif
    sus_table[number]=my_id;
    number++;

    status=rtems_task_suspend(my_id);
    #ifdef DEBUGSWI
      SHOW_DEBUG_INT(status);
      if(status==RTEMS_SUCCESSFUL) SHOW_DEBUG("RTEMS_SUCCESSFUL");
      if(status==RTEMS_INVALID_ID) SHOW_DEBUG("RTEMS_INVALID_ID");
      if(status==RTEMS_ALREADY_SUSPENDED) SHOW_DEBUG("RTEMS_ALREADY_SUSPENDED");
    #endif

    //#ifdef DEBUGSWI
      SHOW_DEBUG("Task resumed:");
      SHOW_DEBUG_INT(my_id);
    //#endif
  }
  else
   break;
 }
 
 #ifdef DEBUGSWII
 SHOW_DEBUG("WAIT: enabling interrupts");
 #endif
 rtems_interrupt_enable(level); 

}

//interrupt handler
void shared_interrupt_handler(){
  
  //#ifdef DEBUGSWI
    SHOW_DEBUG("INTERRUPT: checking for suspended tasks");
    //SHOW_DEBUG("INTERRUPT: disabling interrupts");
  //#endif
  
  register rtems_interrupt_level level;
  rtems_interrupt_disable(level);
  
  for(uint i=0;i<number;i++) 
  {
   if(sus_table[i]!=0){
        register rtems_status_code status;
	unsigned int id=sus_table[i];
	sus_table[i]=0;
	#ifdef DEBUGSWI
	  SHOW_DEBUG("INTERRUPT: resuming task:");
	  SHOW_DEBUG_INT(id);
	#endif
	
	#ifdef DEBUG0
	  printf("INTERRUPT: resuming task %x at %x\n",id,&scratch_table[i]);
	#endif
	
	status=rtems_task_resume(id);
	#ifdef DEBUGSWI
	  SHOW_DEBUG_INT(status);
	  if(status==RTEMS_SUCCESSFUL) SHOW_DEBUG("RTEMS_SUCCESSFUL");
	  if(status==RTEMS_INVALID_ID) SHOW_DEBUG("RTEMS_INVALID_ID");
	  if(status==RTEMS_INCORRECT_STATE) SHOW_DEBUG("RTEMS_INCORRECT_STATE");
	#endif
      }
   } 
   
  number=0;
    
  #ifdef DEBUGSWI
    SHOW_DEBUG("INTERRUPT: enabling interrupts");
  #endif
  rtems_interrupt_enable(level);
  
  //#ifdef DEBUGSWI
    SHOW_DEBUG("INTERRUPT: completed");
  //#endif
  
}

//initialize ss_semaphore system building library data structures
int shared_int_system_init(){

  for(int i=0;i<MAX_SUPPORTED_TASK;i++) sus_table[i]=0;
  number=0;
  
  static rtems_irq_connect_data shared_int_data = { IRQ_SHARED_PIN,
   shared_interrupt_handler, son, soff, sison, 0, 0};
  
  if(BSP_install_rtems_irq_handler(&shared_int_data)==0) return 0;
  shared_int_data.on(&shared_int_data);
 
  return 1;
}
