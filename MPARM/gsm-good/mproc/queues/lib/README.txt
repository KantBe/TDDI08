This Library aim to provide effcient library support in order to allow
flexible instatiation of message queue.

************************
CONFIGURATION PARAMETERS
************************
You have several paramters you can configure in scratch_queue.h:
//define the maximum numebrs of queue instatiable
#define MAXQUEUE 10
//enable dma use for multiple 32-bit memory transfers
#define USEDMA
//use ss_semaphore library instead of scratch_sempahore li
#define USEINTERRUPT
//enable queue instantiation which has data buffer in shar
#define USE_SHARED_BUFFER

//N.B. SCRATCH_SIZE_PLATFORM should match the dimension of the scratch in 
//the platform otherwise the QUEUE_BASE address will result wrong and
//this imply problems in the initialization function 
#define SCRATCH_SIZE_PLATFORM (1024*4) 

************************
ENVIRONMENT VARIABLES
************************
//Base folder of the library
setenv QUEUELIB $HOME/application/que_lib/lib

//all this environment variables are normally subfolder of the base one 
//by changing these you can compile with different version of part
//which compose the library 
setenv SS_SEMAPHORE_LIB_PATH ${QUEUELIB}/ss_semaphore_lib
setenv SCRATCH_QUEUE_LIB_PATH ${QUEUELIB}/scratch_queue_lib
setenv SCRATCH_SEMAPHORE_LIB_PATH ${QUEUELIB}/scratch_semaphore_lib
setenv EXT_INT_LIB_PATH ${QUEUELIB}/ext_int_lib


************************
INITIALIZATION FUNCTIONS
************************
//this function initializes the queue system
//-sha_mutex is the address of shared mutex which control the acces
//  to the global variable current_shared_pointer
//-sha_start is the start address of shared memory
//-sha_size is the size of the shared memory
//-scra_start is the start address of the scratch-pad memory
//-scra_size is the size of the scratch-pad
#ifdef USE_SHARED_BUFFER
void scratch_queue_autoinit_system
(unsigned int sha_mutex=0,unsigned int sha_start=0, unsigned int sha_size=0,
 unsigned int scra_start=0,unsigned int scra_size=0)
#else
void scratch_queue_autoinit_system(unsigned int scra_start=0,unsigned int scra_size=0)
#endif

//At each queue must be assigned an id like tcp/ip protocol in detail: 
//each queue is identifiable by the consumer processor id 
//and an id locally assigned to each consumer queue

//During initialization the producer should know where the consumer
//queue reside so that it can intialize consumer semaphore=>this
//allows to have shynchronization between producer and consumer during
//initialization  

//Queue Producer initialization function
//core_consumer_id is the id of the core where the consumer reside
//queue_id is the consumer queue id
//n_items numbers of maximun object inside the queue
//token_size size of each object in the queue
//bufferzone indocate where the queue buffer has to be stored by
//default is set to 0=> scratch-pad memory
SCRATCH_QUEUE_PRODUCER* scratch_queue_autoinit_producer
(int core_consumer_id, int queue_id, int n_items, int token_size, 
 int bufferzone=0)

//queue_id is the consumer queue id
//allocate_local_buffer when true during initialization memory space
//for store data readed is reserved in the scratch-pad 
SCRATCH_QUEUE_CONSUMER* scratch_queue_autoinit_consumer
(int queue_id, bool allocate_local_buffer)


************************
READ/WRITE FUNCTIONS
************************
//Returns the actual pointer to the scratch
void* getFreeScratchAddress();

//read from the queue and write into a buffer; this function has suspensive semantic
inline void scratch_queue_read(register SCRATCH_QUEUE_CONSUMER *queue_p,register char *buffer)
//read from the queue and write into the consumer local buffer, returned as result; this function has suspensive semantic
inline char* scratch_queue_read(register SCRATCH_QUEUE_CONSUMER *queue_p)
//read from the queue using the dma
inline void scratch_queue_read_dma(register SCRATCH_QUEUE_CONSUMER *queue_p,register char *buffer)
inline char* scratch_queue_read_dma(register SCRATCH_QUEUE_CONSUMER *queue_p)

//write on queue specified data; this function has a suspensive semantic
inline void scratch_queue_write(register SCRATCH_QUEUE_PRODUCER *queue_p,register  char *data)
//write on queue specified data USING DMA; this function has a suspensive semantic
inline void scratch_queue_write_dma(register SCRATCH_QUEUE_PRODUCER *queue_p,register  char *data)

//this function permit to obtain the address of the first queue free buffer
//this function has suspensive semantic
inline char* scratch_queue_getToken_write(register SCRATCH_QUEUE_PRODUCER *queue_p)
//this function "commit" the last "getToken" buffer in the queue
//must be used only sequentially after scratch_queue_getToken_write
inline void scratch_queue_putToken_write(register SCRATCH_QUEUE_PRODUCER *queue_p)
