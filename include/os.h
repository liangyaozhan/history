
/**
 * Last modified Time-stamp: <2012-10-21 09:30:08 Sunday by lyzh>
 *
 * @file    os.h
 * @author  liangyaozhan @2012-10-14
 * @version 0.1.0
 * @brief   ppos header file
 */


#ifndef __TCB_H
#define __TCB_H

#include <stdint.h>
#include "list.h"

/**
 *  @addtogroup kernel
 *  @{
 */

/**
 *  @addtogroup configuration
 *  @{
 */

/**
 *  @name priority_number_configuration
 *  @{
 */
#define MAX_PRIORITY            8  /*!< must be <= 256 and >=1 */
/**
 *  @}
 */

/**
 *  @brief optimize macro
 */
#define likely(x)	__builtin_expect(!!(x), 1)  /*!< likely optimize macro      */
#define unlikely(x)	__builtin_expect(!!(x), 0)  /*!< unlikely optimize macro    */

/**
 *  @brief ROUND DOWN and UP
 */
#define ROUND_DOWN(p, d)        (((int)p - ((d)-1)) & (~(d-1)))
#define ROUND_UP(x, align)  (((int) (x) + (align - 1)) & ~(align - 1))

#if DEBUG>0
#define ASSERT(condiction)                                              \
    do{                                                                 \
        if ( (condiction) )                                             \
            break;                                                      \
        kprintf("ASSERT " #condiction "failed: " __FILE__ ":%d: " ": " "\r\n", __LINE__); \
        while (9);                                                      \
    }while (0)
#else
#define ASSERT(condiction)  do{}while(0)
#define KERNEL_NO_ARG_CHECK
#endif

/**
 *  @addtogroup kernel_structure
 *  @{
 */

/**
 *  @brief soft timer structure definition
 *
 *  this is a link list watchdog like soft timer implementation.
 *  The list is sorted at insertion time by uiTick.
 *  The first element's uiTick is decreased at each tick anounce in the link list.
 */
struct __softtimer
{
    struct list_head node;                          /*!< node to the link list   */
    unsigned int     uiTick;                        /*!< tick count              */
    void (*timeout_func)( struct __softtimer *);    /*!< timeout function        */
};
typedef struct __softtimer softtimer_t;

/**
 *  @brief priority queue node structure
 *
 *  This is a priority queue node with key.
 */
struct __priority_q_node
{
    struct list_head node;          /*!< the node       */
    int              key;           /*!< the key value  */
};
typedef struct __priority_q_node pqn_t;

/**
 *  @brief task control block definition
 *
 *  Task Control Block have many fields.
 *  Task's name is copied to top of the stack by task_init() functoin.
 *  option and safe_count is not used currently.
 *  @sa TASK_API
 */
struct                 __tcb_t;
typedef struct __tcb_t tcb_t;
struct __tcb_t
{
    void             *sp;                   /*!< stack pointor          */
    char             *name;                 /*!< task's name            */
    char             *stack_low;            /*!< stack low pointor      */
    char             *stack_high;           /*!< stack high pointor     */
    int               option;               /*!< for the further        */
    int               current_priority;     /*!< running priority       */
    int               priority;             /*!< normal priority        */
    pqn_t             prio_node;            /*!< node in priority Q     */
    softtimer_t       tick_node;            /*!< node in softtimer Q    */
    struct list_head  sem_node;             /*!< node in pending Q      */
    struct list_head *psem_list;            /*!< pend node if any       */
    struct list_head  mutex_holded_head;    /*!< remember all mutex     */
    struct list_head  task_list_node;       /*!< node in task list      */
    int               err;                  /*!< errno used internal    */
    int               safe_count;           /*!< prevent task deletion  */
    int status;                             /*!< task status            */
};


/**
 *  @brief semaphore definition
 *
 *  @sa SEMAPHORE_API
 */
struct __semaphore_t
{
	union {
		struct __tcb_t     *owner; /*!< owner when it is mutex              */
		unsigned int        count; /*!< counter when it is semB or sem C    */
	}u;
	int                     type;                   /*!< Specifies the type of semaphore     */
	int                     mutex_recurse_count;    /*!< only used when it is mutex          */
	struct list_head        pending_tasks;          /*!< the pending tasks link list head    */
	struct list_head        sem_member_node;        /*!< only used when it is mutex          */
};
typedef struct __semaphore_t semaphore_t;


/**
 *  @brief msgq_t struct definition
 *
 *  @sa  MSGQ_API  
 */
struct __msgq_t
{
    semaphore_t sem_rd;         /*!< read semaphore          */
    semaphore_t sem_wr;         /*!< write semaphore         */
    int         buff_size;      /*!< buffer size             */
    int         count;          /*!< max unit                */
    int         unit_size;      /*!< element size            */
    int         rd;             /*!< read pointor            */
    int         wr;             /*!< write pointor           */
    char        buff[1];        /*!< dynamic allocated       */
};
typedef struct __msgq_t msgq_t;


/**
 *  @}
 */

/**
 *  @defgroup kernel_const_value
 *  @{
 */
/**
 *  @defgroup task_status
 *  @{
 */
#define TASK_READY              0x00
#define TASK_SUSPEND            0x01
#define TASK_DELAYD             0x02
#define TASK_DEAD               0x04
/**
 *  @}
 */
/**
 *  @}
 */

/**
 *  @addtogroup kernel_const_value
 *  @{
 */
/**
 *  @defgroup semaphore_type
 *  @{
 */
#define SEM_TYPE_BINARY 0x01    /*!< semaphore type: binary  */
#define SEM_TYPE_COUNT  0x02    /*!< semaphore type: counter */
#define SEM_TYPE_MUTEX  0x03    /*!< semaphore type: mutex   */
/**
 *  @}
 */
/**
 *  @}
 */


/**
 *  @name wait forever
 *  @{
 */
#define WAIT_FOREVER    ((unsigned int)0xffffffff)
/**
 *  @}
 */

/**
 *  @defgroup API
 *  @{
 */

/**
 *  @defgroup TASK_API
 *  @{
 */

/**
 *  @brief declare task infomation
 *
 *  @par startup example  
 *  @code
 *  void func(char *a, char *b) {
 *  
 *      init_systimer();
 *      while (1) {
 *          kprintf("hello %s %s\n", a, b)
 *          task_delay(1);
 *      }
 *  }
 *  int main()
 *  {
 *      int priority1 = 10;
 *      int priority2 = 20;
 *      TASK_INFO_DECL(static, info1, 1024);
 *      TASK_INFO_DECL(static, info2, 1024);
 *      
 *      arch_interrupt_disable();
 *      kernel_init();
 *      
 *      TASK_INIT( "ta", info1, priority1, 0, func, "task a p1","task a p2" );
 *      TASK_INIT( "tb", info2, priority2, 0, func, "task b p1","task b p2" );
 *      TASK_STARTUP(info1);
 *      TASK_STARTUP(info2);
 *      os_startup();
 *      return 0;
 *  }
 *  @endcode
 */
#define TASK_INFO_DECL(namespace, info, stack_size)  namespace char stack_##info[stack_size]; \
    namespace tcb_t tcb_##info
/**
 *  @brief init task infomation
 *
 *  infomation value 'info' is from TASK_INFO_DECL().
 */
#define TASK_INIT(name, info, priority, func, arg1, arg2)               \
    task_init( &tcb_##info, (name), (priority), 0, stack_##info, stack_##info+sizeof(stack_##info), func, (arg1), (arg2) )
/**
 *  @brief task startup macro.
 *
 *  infomation value 'info' is from TASK_INFO_DECL().
 */
#define TASK_STARTUP(info)  task_startup( &tcb_##info )

/**
 * @brief kernel initialize
 *
 * kernel_init() must be called first of all.
 */
void kernel_init( void );

/**
 * @brief task control block init.
 *
 * @param ptcb          pointor of task control block. You provide it.
 * @param name          name of the task. This string is copied to the top of the stack.
 * @param priority      task normal running priority.
 * @param stack_low     low address of the stack.
 * @param stack_high    high address of the stack.
 * @param pfunc         task entry function.
 * @param arg1          argument 1 to the task.
 * @param arg2          argument 2 to the task.
 *
 *  A task is initialized by this function but not be added to running queue.
 *  A task_starup(ptcb) will add a initialized task to running queue.
 *  So you must use function task_startup() to start a initialized task.
 * @sa task_starup();
 */
void task_init(tcb_t      *ptcb,
               const char *name,
               int         priority, /* priority of new task */
               int         option, /* task option word */
               char *      stack_low,
               char *      stack_high,
               void       *pfunc, /* entry point of new task */
               void       *arg1, /* 1st of 10 req'd args to pass to entryPt */
               void       *arg2);

/**
 *  @brief task start up.
 *
 *  add a task to running queue.
 */
int task_startup( tcb_t *ptcb );

/**
 *  @brief starting up operating system.
 *
 *  calling this function will start the scheduler, and switch to
 *  the highest priority task created. So, task must be created and started up
 *  before calling this function.
 */
void os_startup( void );

/**
 *  @brief task delay
 *
 *  delay tick.
 */
void task_delay( int tick );

int task_safe( void );
int task_unsafe( void );

/**
 *  @brief current task name
 */
#define CURRENT_TASK_NAME() (ptcb_current->name+0)

/**
 *  @brief current task priority
 */
#define CURRENT_TASK_PRIORITY() (ptcb_current->priority+0)

/**
 *  @brief current task errno
 */
#define CURRENT_TASK_ERR() (ptcb_current->err+0)

/**
 *  @}
 */

/**
 *  @defgroup SEMAPHORE_API
 *  @{
 */

/**
 *  @brief semaphore declaration macro
 *
 *  @par example  
 *  @code
 *  semaphore_t semb;
 *  semaphore_t semc;
 *  semaphore_t mutex;
 *  void do_init( void ) {
 *      sem_counter_init( &semc, init_count );
 *      sem_binary_init( &semb, init_count ); /@ 0 or 1 @/
 *      mutex_init( &mutex );
 *  }
 *  
 *  void use_it( void ) {
 *      sem_counter_take( &semc, tick );
 *      ...
 *      sem_binary_take( &semb, tick );
 *      ...
 *      mutex_lock( &mutex );
 *      ...
 *      mutex_unlock( &mutex );
 *      ...
 *  }
 *  @endcode
 */
#define SEM_DECL( sem, t, init)                                         \
	semaphore_t sem = {                                                 \
		.u.count = init,                                                \
        .type=t,                                                        \
		.mutex_recurse_count = 0,                                       \
		.pending_tasks = LIST_HEAD_INIT(sem.pending_tasks),             \
		.sem_member_node = LIST_HEAD_INIT(sem.sem_member_node),         \
	}

/**
 *  @brief semaphore binary declaration macro.
 */
#define SEM_BINARY_DECL(name, init_value)   SEM_DECL(name, SEM_TYPE_BINARY, init_value)

/**
 *  @brief semaphore counter declaration macro.
 */
#define SEM_COUNT_DECL(name, init_value)    SEM_DECL(name, SEM_TYPE_COUNT, init_value)

/**
 *  @brief mutex declaration macro.
 */
#define MUTEX_DECL(mutex)                   SEM_DECL(mutex, SEM_TYPE_MUTEX, 0)

/**
 *  @brief semaphore counter initialize
 *
 *  @param  semid       semaphore counter piontor.
 *  @param  InitCount   initialize value.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  sem_counter_init( semaphore_t *semid, int InitCount );

/**
 *  @brief semaphore binary initialize
 *
 *  @param  semid       semaphore binary piontor.
 *  @param  InitCount   initialize value.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  sem_binary_init( semaphore_t *semid, int InitCount );

/**
 *  @brief semaphore counter take
 *
 *  @param  semid       semaphore counter piontor.
 *  @param  tick        max waitting tick. -1 indicates forever.
 *                      @sa sys_clkrate_get().
 *                      if tick == 0, no waiting is performed.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  sem_counter_take( semaphore_t *semid, unsigned int tick );

/**
 *  @brief semaphore counter initialize
 *
 *  @param  semid       semaphore counter piontor.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  sem_counter_give( semaphore_t *semid );

/**
 *  @brief semaphore counter initialize
 *
 *  @param  semid       semaphore binary piontor.
 *  @param  tick        max waitting tick. -1 indicates forever.
 *                      @sa sys_clkrate_get().
 *                      if tick == 0, no waiting is performed.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  sem_binary_take( semaphore_t *semid, unsigned int tick );

/**
 *  @brief semaphore counter initialize
 *
 *  @param  semid       semaphore binary piontor.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  sem_binary_give( semaphore_t *semid );

/**
 *  @brief mutex initialize
 *
 *  @param  semid       mutex pointor
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  mutex_init( semaphore_t *semid );

/**
 *  @brief mutex lock
 *
 *  @param  semid       mutex pointor
 *  @param  tick        max waitting tick. -1 indicates forever.
 *                      @sa sys_clkrate_get().
 *                      if tick == 0, no waiting is performed.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  mutex_lock( semaphore_t *semid, unsigned int tick );

/**
 *  @brief mutex unlock
 *
 *  @param  semid       semaphore counter piontor.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  mutex_unlock( semaphore_t *semid );
/**
 *  @}
 */


/**
 *  \defgroup MSGQ_API
 *  @{
 */

/**
 *  @brief declare & init msgq
 *  
 *  declare a msg queue and init.
 *  large code size.
 *  please use MSGQ_DECL_NO_INIT() & MSGQ_DO_INIT()
 *
 *  @par example  
 *  @code
 *  
 *  MSGQ_DECL_INIT( mymsgq, sizeof(int), 10 );
 *  int func_init( void )
 *  {
 *      /@ do nothing to init @/
 *      return 0;
 *  }
 *  int func_send( int *buff )
 *  {
 *      return msgq_send( (msgq_t*)&mymsgq, buff, sizeof(int), 1000 );
 *  }
 *  int func_recieve( int *buff )
 *  {
 *      return msgq_recieve( (msgq_t*)mymsgq, buff, sizeof(int), -1, 0);
 *  }
 *  @endcode
 */

#define MSGQ_DECL_INIT(name, unitsize, cnt)                             \
    struct __msgq__t##name {                                            \
        msgq_t msgq;char buff[(unitsize)*(cnt)];                        \
    } name = {                                                          \
        .msgq = {                                                       \
            .sem_rd = {                                                 \
                .u.count = 0,                                           \
                .type = SEM_TYPE_COUNT,                                 \
                .mutex_recurse_count = 0,                               \
                .pending_tasks = LIST_HEAD_INIT((name).msgq.sem_rd.pending_tasks), \
                .sem_member_node = LIST_HEAD_INIT((name).msgq.sem_rd.sem_member_node), \
            },                                                          \
            .sem_wr = {                                                 \
                .u.count = cnt,                                         \
                .type = SEM_TYPE_COUNT,                                 \
                .mutex_recurse_count = 0,                               \
                .pending_tasks = LIST_HEAD_INIT((name).msgq.sem_wr.pending_tasks), \
                .sem_member_node = LIST_HEAD_INIT((name).msgq.sem_wr.sem_member_node), \
            },                                                          \
            .buff_size = sizeof((name).buff),                           \
            .count = cnt,                                               \
            .unit_size = unitsize,                                      \
            .rd = 0,                                                    \
            .wr = 0,                                                    \
        },                                                              \
    }

/**
 *  @brief declare & init msgq
 *  
 *  this is recommemded way to init msgq ( reducing code's size ):
 *  
 *  @par example  
 *  @code
 *  
 *  MSGQ_DECL_NO_INIT( mymsgq, sizeof(int)*10 );
 *  int func_init( void )
 *  {
 *      return MSGQ_DO_INIT( mymsgq, sizeof(int) );
 *  }
 *  int func_send( int *buff )
 *  {
 *      return msgq_send( (msgq_t*)&mymsgq, buff, sizeof(int), 1000 );
 *  }
 *  int func_recieve( int *buff )
 *  {
 *      return msgq_recieve( (msgq_t*)mymsgq, buff, sizeof(int), -1, 0);
 *  }
 *  @endcode
 */
#define MSGQ_DECL_NO_INIT(name, buffersize )    \
    struct __msgq__t##name {                    \
        msgq_t msgq;char buff[buffersize];      \
    } name
#define MSGQ_DO_INIT(name, unitsize)  msgq_init(&name.msgq, sizeof(name), unitsize)

/**
 *  @brief usage of msgq_init()
 *
 *  @par example  
 *  @code
 *      #define COUNT 10
 *      #define M sizeof(struct _yourbase_type)*COUNT
 *      struct _yourbase_type {
 *          int a;int b;
 *      };
 *      struct _yourtype_t {
 *          msgq_t msgq;
 *          char buffer[M];
 *      }msgq_var;
 *      void func_init( void ){
 *          msgq_init( &msgq_var.msgq, sizeof(msgq_var), sizeof(struct _yourbase_type));
 *      }
 *  @endcode
 */
int msgq_init( msgq_t *pmsgq, int objsize, int unit_size );

/**
 *  @brief recieve msg from a msgQ
 *  @param pmsgq     a pointor to the msgQ.(the return value of function msgq_create)
 *  @param buff      the memory to store the msg. It can be NULL. if
 *                   it is NULL, it just remove one message from the head.
 *  @param buff_size the buffer size.
 *  @param tick      the max time to wait if there is no message.
 *                   if pass -1 to this, it will wait forever.
 *  @param flag      0: copy the msg to the buffer and then remove it.
 *                   1: copy the msg to the buffer, but do not remove it.
 *  @retval -1       error, please check errno. if errno == ETIME, it means Timer expired,
 *                   if errno == ENOMEM, it mean buffer_size if not enough.
 *  @retval 0        recieve successfully.
 */
int msgq_recieve( msgq_t *pmsgq, void *buff, int buff_size, int tick, int flag );

/**
 *  @brief send message to a the message Q.
 *  @param pmsgq     a pointor to the msgQ.(the return value of function msgq_create)
 *  @param buff      the message to be sent.
 *  @prarm size      the size of the message to be sent in bytes.
 *  @param tick      if the msgQ is not full, this function will return immedately, else it
 *                   will block some tick. Set it to -1 if you want to wait forever.
 *  @retval 0        OK
 *  @retval -1       the msgQ is full or pmsgQ is not valid. check errno for details.
 *                   errno:
 *                     EINVAL - Invalid argument
 *                     ETIME  - Timer expired
 *  @retval -2       invalid
 */
int msgq_send( msgq_t *pmsgq, const void *buff, int size, int tick );

/**
 *  @brief msgq clear internal count to 0(empty).
 */
int msgq_clear( msgq_t *pmsgq );

/**
 *  @}
 */

/**
 *  @defgroup OTHER_API
 *  @{
 */

void arch_interrupt_enable( int old );
int  arch_interrupt_disable( void );
int  kprintf ( const char* str, ... );

extern int is_int_context;
extern void schedule(void);
#define IS_INT_CONTEXT()        (is_int_context>0)
#define ENTER_INT_CONTEXT()     (++is_int_context)
#define EXIT_INT_CONTEXT()      (--is_int_context, schedule())


/**
 * @brief task list head
 *
 * internal used currently
 */
extern struct list_head  GlistAllTaskListHead;
/**
 * @brief pontor of current task control bock.
 *
 * @ATTENTION  you should NEVER modify it. 
 */
extern tcb_t *ptcb_current;

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
 *  @}
 */


/**
 *  @}
 */

/**
 *  @}
 */

/**
 *  @}
 */

#endif

