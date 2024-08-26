/***********************************HEADER INFO********************************/
/*  
*   FILE NAME   :  scratch_semaphore.h
*
*   DATE        :  27/02/2004
*			                   		
*   AUTHOR      :  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*	
*   VERSION     :  1.0
*
*   CONTENTS    :  Prototypes and defines of scratch semaphore primitives
*
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile int * SCRATCH_SEMAPHORE;

inline int scratch_sem_wait(SCRATCH_SEMAPHORE my_semaphore);

inline void scratch_sem_signal(SCRATCH_SEMAPHORE my_semaphore);

inline void scratch_sem_init(SCRATCH_SEMAPHORE my_semaphore,int value);

#ifdef __cplusplus
}
#endif
