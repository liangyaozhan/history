/* Last modified Time-stamp: <2012-11-05 16:05:54 Monday by liangyaozhan>
 * 
 * Copyright (C) 2012 liangyaozhan <ivws02@gmail.com>
 * 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __TCB_H
#define __TCB_H

#include <stdint.h>
#include "list.h"
#include "rtk_config.h"

#define size_t uint32_t

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
/*#define MAX_PRIORITY            8  *//*!< must be <= 256 and >=1 */
/**
 *  @}
 */

/**
 *  @brief optimize macro
 */
#define likely(x)    __builtin_expect(!!(x), 1)  /*!< likely optimize macro      */
#define unlikely(x)    __builtin_expect(!!(x), 0)  /*!< unlikely optimize macro    */

/**
 *  @brief ROUND DOWN and UP
 */
#define ROUND_DOWN(p, d)        (((int)p) & (~(d-1)))
#define ROUND_UP(x, align)  (((int) (x) + (align - 1)) & ~(align - 1))

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
    const char       *name;                 /*!< task's name            */
    char             *stack_low;            /*!< stack low pointor      */
    char             *stack_high;           /*!< stack high pointor     */
    pqn_t             prio_node;            /*!< node in priority Q     */
    softtimer_t       tick_node;            /*!< node in softtimer Q    */
    struct list_head  sem_node;             /*!< node in pending Q      */
    struct list_head *psem_list;            /*!< pend node if any       */
#if CONFIG_MUTEX_EN
    struct list_head  mutex_holded_head;    /*!< remember all mutex     */
    int               priority;             /*!< normal priority        */
#endif
    struct list_head  task_list_node;       /*!< node in task list      */
    int               option;               /*!< for the further        */
    int               current_priority;     /*!< running priority       */
    int               err;                  /*!< errno used internal    */
#if CONFIG_TASK_TERMINATE_EN
    int               safe_count;           /*!< prevent task deletion  */
#endif
    int               status;                             /*!< task status            */
};


/**
 *  @brief semaphore definition
 *
 *  @sa SEMAPHORE_API
 */
struct __semaphore_t
{
    union {
        unsigned int        count; /*!< counter when it is semB or sem C    */
        struct __tcb_t     *owner; /*!< owner when it is mutex              */
    }u;
    struct list_head        pending_tasks;          /*!< the pending tasks link list head    */
    unsigned char           type;                   /*!< Specifies the type of semaphore     */
};
typedef struct __semaphore_t semaphore_t;

/**
 *  @brief mutex struct
 */
struct __mutex_t
{
    semaphore_t             s;
    struct list_head        sem_member_node;        /*!< only used when it is mutex          */
    int                     mutex_recurse_count;    /*!< only used when it is mutex          */
};
typedef struct __mutex_t mutex_t;


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
    char       *buff;        /*!< dynamic allocated       */
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
#define TASK_PENDING            0x01
#define TASK_DELAY              0x02
#define TASK_DEAD               0xF0
#define TASK_PREPARED           0x5C
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
#define SEM_TYPE_BINARY     0x01    /*!< semaphore type: binary  */
#define SEM_TYPE_COUNTER    0x02    /*!< semaphore type: counter */
#define SEM_TYPE_MUTEX      0x03    /*!< semaphore type: mutex   */
/**
 *  @}
 */
/**
 *  @}
 */


/**
 *  @name waitforever
 *  @{
 */
#define WAIT_FOREVER    ((unsigned int)0xffffffff)
/**
 *  @}
 */

/**
 *  @name namespace
 *  @{
 */
#define GLOBAL          /*!< global name space used for MSGQ_DECL_xxx...    */
#define IMPORT  extern  /*!< import name space used for MSGQ_DECL_xxx...    */
#define LOCAL   static  /*!< static name space used for MSGQ_DECL_xxx...    */
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
 *      TASK_INIT( "ta", info1, priority1, func, "task a p1","task a p2" );
 *      TASK_INIT( "tb", info2, priority2, func, "task b p1","task b p2" );
 *      TASK_STARTUP(info1);
 *      TASK_STARTUP(info2);
 *      os_startup();
 *      return 0;
 *  }
 *  @endcode
 */
#define TASK_INFO_DECL(zone, info, stack_size) zone struct __taskinfo_##info {tcb_t tcb; char stack[stack_size]; }info
/**
 *  @brief init task infomation
 *
 *  infomation value 'info' is from TASK_INFO_DECL().
 */
#define TASK_INIT(name, info, priority, func, arg1, arg2)               \
    task_init( &info.tcb, (name), (priority), 0, info.stack, info.stack+sizeof(info.stack)-1, func, (arg1), (arg2) )
/**
 *  @brief task startup macro.
 *
 *  infomation value 'info' is from TASK_INFO_DECL().
 */
#define TASK_STARTUP(info)  task_startup( &info.tcb )

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
int task_priority_set( tcb_t *ptcb, unsigned int priority );
/**
 *  @brief starting up operating system.
 *
 *  calling this function will start the scheduler, and switch to
 *  the highest priority task created. So, task must be created and started up
 *  before calling this function.
 */
void os_startup( void );

/**
 *  @brief set task priority 
 *  @fn task_priority_set
 *  @param[in]  ptcb            task control block pointor. If NULL, current task's
 *                              priority will be change.
 *  @param[in]  new_priority    new priority.
 *  @return     0               successfully.
 *  @return     -EINVAL         Invalid argument.
 *  @return     -EPERM          Permission denied. The task is not startup yet.
 *
 * new_priority:         P0       P1   P3
 *                        |       |     |
 *  0(high) ==============================================>> 256(low priority)
 *                                ^
 *                                |
 *                         current priority
 */
int task_priority_set( tcb_t *ptcb, unsigned int priority );


/**
 *  @brief stop a task and deinitialize.
 *
 *  the task may be running or pending.
 */
int task_terminate( tcb_t *ptcb );

/**
 *  @brief task delay
 *
 *  delay tick.
 */
void task_delay( int tick );
unsigned int tick_get( void );

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
 *  mutex_t mutex;
 *  void do_init( void ) {
 *      semc_init( &semc, init_count );
 *      semb_init( &semb, init_count ); /@ 0 or 1 @/
 *      mutex_init( &mutex );
 *  }
 *  
 *  void use_it( void ) {
 *      semc_take( &semc, tick );
 *      ...
 *      semb_take( &semb, tick );
 *      ...
 *      mutex_lock( &mutex );
 *      ...
 *      mutex_unlock( &mutex );
 *      ...
 *  }
 *  @endcode
 */
#define SEM_DECL( sem, t, init)                 \
    semaphore_t sem; semaphore_t sem = {                         \
        {init},                                 \
        LIST_HEAD_INIT(sem.pending_tasks),      \
        t,                                      \
    }

/**
 *  @brief semaphore binary declaration macro.
 */
#define SEM_BINARY_DECL(zone, name, init_value)  zone SEM_DECL(name, SEM_TYPE_BINARY, init_value)

/**
 *  @brief semaphore counter declaration macro.
 */
#define SEM_COUNT_DECL(zone, name, init_value)   zone SEM_DECL(name, SEM_TYPE_COUNTER, init_value)

/**
 *  @brief mutex declaration macro.
 */
#define MUTEX_DECL(zone, var)                                  \
    zone mutex_t var={                                        \
        {                                                \
            {0}, LIST_HEAD_INIT((var).s.pending_tasks),  \
            SEM_TYPE_MUTEX,                              \
        },                                               \
        LIST_HEAD_INIT((var).sem_member_node),           \
        0,                                               \
    }
    
/**
 *  @brief semaphore counter initialize
 *
 *  @param  semid       semaphore counter piontor.
 *  @param  InitCount   initialize value.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  semc_init( semaphore_t *semid, int InitCount );

/**
 *  @brief semaphore counter deinitialize
 */
int  semc_terminate( semaphore_t*semid );

/**
 *  @brief semaphore binary initialize
 *
 *  @param  semid       semaphore binary piontor.
 *  @param  InitCount   initialize value.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  semb_init( semaphore_t *semid, int InitCount );

/**
 *  @brief semaphore binary deinitialize
 */
int  semb_terminate( semaphore_t*semid );

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
int  semc_take( semaphore_t *semid, unsigned int tick );

/**
 *  @brief semaphore counter initialize
 *
 *  @param  semid       semaphore counter piontor.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  semc_give( semaphore_t *semid );

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
int  semb_take( semaphore_t *semid, unsigned int tick );

/**
 *  @brief semaphore counter initialize
 *
 *  @param  semid       semaphore binary piontor.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  semb_give( semaphore_t *semid );

/**
 *  @brief mutex initialize
 *
 *  @param  semid       mutex pointor
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  mutex_init( mutex_t *semid );

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
int  mutex_lock( mutex_t *semid, unsigned int tick );

/**
 *  @brief mutex unlock
 *
 *  @param  semid       semaphore counter piontor.
 *  @return 0           OK.
 *  @return -1          FAILED.
 */
int  mutex_unlock( mutex_t *semid );

/**
 *  @brief mutex deinitialize
 *  
 */
int mutex_terminate( mutex_t *mutex );
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

#define MSGQ_DECL_INIT(zone, name, unitsize, cnt)                             \
    zone char __msgqbuff##name[(unitsize)*(cnt)];                  \
    zone msgq_t name; msgq_t name = {                                           \
        /* .sem_rd = , */                                               \
        {{0,}, LIST_HEAD_INIT((name.sem_rd.pending_tasks)),  SEM_TYPE_COUNTER},    \
        /* .sem_wr = , */                                               \
        {{cnt,},LIST_HEAD_INIT((name.sem_wr.pending_tasks)), SEM_TYPE_COUNTER},  \
        /* .buff_size = sizeof((name).buff),  */                        \
        /* .count = cnt, */                                             \
        /* .unit_size = unitsize, */                                    \
        (unitsize)*(cnt),cnt,unitsize,                                  \
        /* .rd = 0, */                                                  \
        /* .wr = 0, */                                                  \
        0,0,                                                            \
        /* buff*/                                                       \
        __msgqbuff##name,                                               \
    }

/**
 *  @brief declare & init msgq
 *  
 *  this is recommemded way to init msgq ( reducing code's size ):
 *  
 *  @par example  
 *  @code
 *  
 *  MSGQ_DECL_NO_INIT( static, mymsgq, sizeof(int)*10 );
 *  int func_init( void )
 *  {
 *      return MSGQ_DO_INIT( mymsgq, sizeof(int) );
 *  }
 *  int func_send( int *buff )
 *  {
 *      return msgq_send( &mymsgq, buff, sizeof(int), 1000 );
 *  }
 *  int func_recieve( int *buff )
 *  {
 *      return msgq_recieve( (msgq_t*)mymsgq, buff, sizeof(int), -1, 0);
 *  }
 *  @endcode
 */
#define MSGQ_DECL_NO_INIT(namespace, name, buffersize )                 \
    namespace char __msgqbuff##name[buffersize];                        \
    namespace msgq_t name
#define MSGQ_DO_INIT(name, unitsize)  msgq_init( &name, __msgqbuff##name, sizeof(__msgqbuff##name), unitsize)

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
 *      char buffer[M];
 *      msgq_t msgq_var;
 *      void func_init( void ){
 *          msgq_init( &msgq_var, buffer, sizeof(buffer), (struct _yourbase_type));
 *      }
 *  @endcode
 */
int msgq_init( msgq_t *pmsgq, void *buff, int buffer_size, int unit_size );

int msgq_terminate( msgq_t *pmsgq );


/**
 *  @brief recieve msg from a msgQ
 *  @param pmsgq     a pointor to the msgQ.(the return value of function msgq_create)
 *  @param buff      the memory to store the msg. It can be NULL. if
 *                   it is NULL, it just remove one message from the head.
 *  @param buff_size the buffer size.
 *  @param tick      the max time to wait if there is no message.
 *                   if pass -1 to this, it will wait forever.
 *  @retval -1       error, please check errno. if errno == ETIME, it means Timer expired,
 *                   if errno == ENOMEM, it mean buffer_size if not enough.
 *  @retval 0        recieve successfully.
 */
int msgq_receive( msgq_t *pmsgq, void *buff, int buff_size, int tick );

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
extern struct list_head g_systerm_tasks_head;
/**
 * @brief pontor of current task control bock.
 *
 * @ATTENTION  you should NEVER modify it. 
 */
extern tcb_t *ptcb_current;

/**
 *  @brief Find First bit Set
 */
int rtk_ffs( register unsigned int q );

/**
 *  @brief standard memcpy
 */
void *memcpy(void *dst0, const void *src0, int len0);


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

