/* rtklib.h --- rtk libs
 */

#ifndef INCLUDED_RTKLIB_H
#define INCLUDED_RTKLIB_H 1



#if DEBUG>0
#define ASSERT(condiction)                                              \
    do{                                                                 \
        if ( (condiction) )                                             \
            break;                                                      \
        kprintf("ASSERT " #condiction "failed: " __FILE__ ":%d: " ": " "\r\n", __LINE__); \
        while (9);                                                      \
    }while (0)
#define KERNEL_ARG_CHECK_EN 1
#else
#define ASSERT(condiction)  do{}while(0)
#define KERNEL_ARG_CHECK_EN 0
#endif



/**
 *  @brief Find First bit Set
 */
int rtk_ffs( unsigned int q );

/**
 *  @brief standard memcpy
 */
void *memcpy(void *dst0, const void *src0, int len0);

/**
 *  @brief standard memset
 */
void *memset(void *m, int c, int n);
/**
 *  @brief standard strlen
 */
int strlen(const char*);

/**
 * Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param size is the minimum size of the requested block in bytes.
 *
 * @return pointer to allocated memory or NULL if no free memory was found.
 */
void *malloc(unsigned int size);

/**
 * This function will change the previously allocated memory block.
 *
 * @param rmem pointer to memory allocated by malloc
 * @param newsize the required new size
 *
 * @return the changed memory block address
 */
void *realloc( void *rmem, unsigned int newsize );


void free(void *rmem);

struct rtk_mutex     *mutex_create( void );
void         mutex_delete( struct rtk_mutex *mutex );
struct rtk_semaphore *semc_create( int init_count );
void         semc_delete( struct rtk_semaphore *semid );
struct rtk_semaphore *semb_create( int init_count );
void         semb_delete( struct rtk_semaphore *semid );
struct rtk_msgq      *msgq_create( int element_size, int element_count );
void         msgq_delete( struct rtk_msgq *pmsgq );

struct rtk_tcb       *task_create(const char *name,
                                  int         priority,
                                  int         stack_size,
                                  int         option,
                                  void       *pfunc,
                                  void       *arg1,
                                  void       *arg2);


#endif /* INCLUDED_RTKLIB_H */

