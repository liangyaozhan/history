/* rtklib.h --- rtk libs
 */

#ifndef INCLUDED_RTKLIB_H
#define INCLUDED_RTKLIB_H 1

/**
 *  @brief Find First bit Set
 */
int ffs( unsigned int q );

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
void *malloc(size_t size);

/**
 * This function will change the previously allocated memory block.
 *
 * @param rmem pointer to memory allocated by malloc
 * @param newsize the required new size
 *
 * @return the changed memory block address
 */
void *realloc(void *rmem, size_t newsize);


void free(void *rmem);

mutex_t     *mutex_create( void );
void         mutex_delete( mutex_t *mutex );
semaphore_t *semc_create( int init_count );
void         semc_delete( semaphore_t *semid );
semaphore_t *semb_create( int init_count );
void         semb_delete( semaphore_t *semid );
msgq_t      *msgq_create( int element_size, int element_count );
void         msgq_delete( msgq_t *pmsgq );

tcb_t       *task_create(const char *name,
                         int         priority,
                         int         stack_size,
                         int         option,
                         void       *pfunc,
                         void       *arg1,
                         void       *arg2);


#endif /* INCLUDED_RTKLIB_H */

