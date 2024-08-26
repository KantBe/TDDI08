/*
* ss_semaphore: suspensive scratch semaphore library for MPARM.
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 2.4
*/

typedef volatile int* SS_SEMAPHORE;

int ss_sem_system_init(unsigned int queue_address, unsigned int scratch_address, int num);

void ss_sem_wait(SS_SEMAPHORE my_semaphore);

unsigned int ss_sem_wait_read(SS_SEMAPHORE my_semaphore);

void ss_sem_signal(SS_SEMAPHORE my_semaphore);

void ss_sem_init(SS_SEMAPHORE my_semaphore,int value);
