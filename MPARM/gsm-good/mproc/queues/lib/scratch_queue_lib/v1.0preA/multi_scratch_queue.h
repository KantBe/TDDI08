#include "scratch_queue.h"

#define MAXQUEUE 10

void scratch_queue_autoinit_system(void);

SCRATCH_QUEUE* scratch_queue_autoinit_producer(	int core_consumer_id,
						int queue_id,
						int n_items,
						int token_size);

void scratch_queue_autoinit_consumer(SCRATCH_QUEUE *queue_cons,int queue_id);
