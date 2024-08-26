/*
* multi_scratch_queue: initialization of queue implemented on suspensive semaphore
* Poggiali Antonio
* Poletti Francesco
* 14.7.2004 v 2.0
* Uses ss_semaphore library, scratch_queue, scratch_semaphore
* 
* Original code by  Francesco Menichelli (menichelli@mail.die.uniroma1.it)
*/

#include "scratch_queue.h"

#define MAXQUEUE 10

void scratch_queue_autoinit_system(void);

SCRATCH_QUEUE_PRODUCER* scratch_queue_autoinit_producer(int core_consumer_id, int queue_id, int n_items, int token_size);

SCRATCH_QUEUE_CONSUMER* scratch_queue_autoinit_consumer(int queue_id, int allocate_local_buffer);
