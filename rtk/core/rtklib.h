/* rtklib.h --- rtk libs
 */

#ifndef INCLUDED_RTKLIB_H
#define INCLUDED_RTKLIB_H 1

#include "rtk_config.h"

int rtk_sprintf( char *buff, const char* str, ... );

/**
 *  @brief Find First bit Set
 */
int rtk_ffs( unsigned int q );
void *rtk_malloc(unsigned int size);
void *rtk_realloc( void *rmem, unsigned int newsize );
void rtk_free(void *rmem);


struct rtk_task;
struct rtk_semaphore;
struct rtk_mutex;
struct rtk_msgq;
struct rtk_tick;

/* APIs */


struct rtk_task *task_create(const char *name,
                             int         priority,
                             int         stack_size,
                             int         option,
                             void       *pfunc,
                             void       *arg1,
                             void       *arg2);
int              task_delete( struct rtk_task *task );
struct rtk_task *task_like( void *pfunc,void *arg,int   priority );
void             task_delay( int tick );
int              task_safe( void );
int              task_unsafe( void );
const char      *task_name( const struct rtk_task *task);
int              task_priority( const struct rtk_task *task);
int              task_errno( const struct rtk_task *task);
int              task_priority_set( struct rtk_task *task, unsigned int priority );
int              task_startup( struct rtk_task *task );
int              task_status( const struct rtk_task *task );
struct rtk_task *task_self(void);
int              task_terminate( struct rtk_task *task );


struct rtk_mutex     *mutex_create( void );
struct rtk_semaphore *semc_create( int init_count );
struct rtk_semaphore *semb_create( int init_count );
struct rtk_msgq      *msgq_create( int element_size, int element_count );
void                  mutex_delete( struct rtk_mutex *mutex );
void                  semc_delete( struct rtk_semaphore *semid );
void                  semb_delete( struct rtk_semaphore *semid );
void                  msgq_delete( struct rtk_msgq *pmsgq );

unsigned int tick_get( void );


void arch_interrupt_enable( int old );
int  arch_interrupt_disable( void );

struct rtk_tick *rtk_tick_down_counter_create(void);
int rtk_tick_down_counter_set_func( struct rtk_tick *_this, void (*func)(void*), void*arg );
void rtk_tick_down_counter_start( struct rtk_tick *_this, unsigned int tick );
void rtk_tick_down_counter_stop ( struct rtk_tick *_this );
void rtk_tick_down_counter_delete(struct rtk_tick *_this);

int  semc_init( struct rtk_semaphore *semid, int InitCount );
int  semc_terminate( struct rtk_semaphore*semid );
int  semb_init( struct rtk_semaphore *semid, int InitCount );
int  semb_terminate( struct rtk_semaphore*semid );
int  semc_take( struct rtk_semaphore *semid, unsigned int tick );
int  semc_give( struct rtk_semaphore *semid );
int  semb_take( struct rtk_semaphore *semid, unsigned int tick );
int  semb_give( struct rtk_semaphore *semid );

int  mutex_init( struct rtk_mutex *semid );
int  mutex_lock( struct rtk_mutex *semid, unsigned int tick );
int  mutex_unlock( struct rtk_mutex *semid );
int mutex_terminate( struct rtk_mutex *mutex );

struct rtk_msgq *msgq_init( struct rtk_msgq *pmsgq, void *buff, int buffer_size, int unit_size );
int msgq_clear( struct rtk_msgq *pmsgq );
int msgq_send( struct rtk_msgq *pmsgq, const void *buff, int size, int tick );
int msgq_receive( struct rtk_msgq *pmsgq, void *buff, int buff_size, int tick );
int msgq_terminate( struct rtk_msgq *pmsgq );


#ifndef SEM_TYPE_NULL
#define SEM_TYPE_NULL       0x00    /*!< semaphore type: NULL    */
#endif
#ifndef SEM_TYPE_BINARY
#define SEM_TYPE_BINARY     0x01    /*!< semaphore type: binary  */
#endif
#ifndef SEM_TYPE_COUNTER
#define SEM_TYPE_COUNTER    0x02    /*!< semaphore type: counter */
#endif
#ifndef SEM_TYPE_MUTEX
#define SEM_TYPE_MUTEX      0x03    /*!< semaphore type: mutex   */
#endif
#ifndef WAIT_FOREVER
#define WAIT_FOREVER    ((unsigned int)0xffffffff)
#endif
#ifndef TASK_READY
#define TASK_READY              0x00
#endif
#ifndef TASK_PENDING
#define TASK_PENDING            0x01
#endif
#ifndef TASK_DELAY
#define TASK_DELAY              0x02
#endif
#ifndef TASK_DEAD
#define TASK_DEAD               0xF0
#endif
#ifndef TASK_PREPARED
#define TASK_PREPARED           0x5C
#endif

#endif /* INCLUDED_RTKLIB_H */

