/***********************************FILE INFO********************************/
/*  
*   FILE NAME   :  scratch_semaphore.c
*
*   DATE        :  27/02/2004
*			                   		
*   AUTHOR      :  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*	
*   VERSION     :  1.0
*
*   CONTENTS    :  Implementations of scratch semaphore primitives
*
*/

/*
   NOTE	:  This functions deal directly with hardware, so do not modify unless you know exactly how the hardware works
*/


#include "scratch_semaphore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 
scratch_sem_wait: Non blocking wait on a semaphore, return its value. 
It is assumed that the semaphore automatically decrements itself after the read if value>0
*/ 

//modifica static per compilazione cpp
inline int scratch_sem_wait(SCRATCH_SEMAPHORE my_semaphore)
{
	return (*my_semaphore);
}

/* 
scratch_sem_signal: signal on a semaphore
The semaphore increment itself on a writing of a 0 value
*/ 

inline void scratch_sem_signal(SCRATCH_SEMAPHORE my_semaphore)
{
        *my_semaphore = 0; 
}


/* 
scratch_sem_init: initialize semaphore value
the hardware initializes the semaphore to value-1 so value is incremented before writing
*/

inline void scratch_sem_init(SCRATCH_SEMAPHORE my_semaphore,int value)
{
       *my_semaphore = value+1;
}

#ifdef __cplusplus
}
#endif
