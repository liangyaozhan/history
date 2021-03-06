/* Last modified Time-stamp: <2014-08-10 13:07:28, by lyzh>
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

#ifndef __RTK_H
#define __RTK_H 1


#ifdef __cplusplus
extern "C" {
#endif                                                                  /* __cplusplus                  */


#include <stdint.h>
#include "list.h"

#include "rtk_config.h"

/**
 *  @addtogroup kernel
 *  @{
 */

/**
 *  @addtogroup kernel_structure
 *  @{
 */


/**
 *  @brief soft timer structure definition
 *
 *  this is a link list watchdog like soft timer implementation.
 *  The list is sorted at insertion time by 'tick'.
 *  The first element's uiTick is decreased at each tick anounce in the link list.
 */
struct  rtk_tick
{
    struct list_head  node;            /*!< node to the link list     */
    unsigned int      tick;            /*!< tick count                */
    void (*timeout_callback)( void *); /*!< timeout callback function */
    void             *arg;
};


/**
 *  @brief priority queue node structure
 *
 *  This is a priority queue node with key.
 */
struct rtk_private_priority_q_node
{
    struct list_head node;          /*!< the node       */
    int              key;           /*!< the key value  */
};

/**
 *  @brief task control block definition
 *
 *  Task Control Block have many fields.
 *  Task's name is just a pointor.
 *  option and safe_count is not used currently.
 *  @sa TASK_API
 */
struct rtk_task
{
    void                               *sp;                /*!< stack pointor,        */
    const char                         *name;              /*!< task's name           */
    char                               *stack_low;         /*!< stack low pointor     */
    char                               *stack_high;        /*!< stack high pointor    */
    struct rtk_private_priority_q_node  prio_node;         /*!< node in priority Q    */
    struct rtk_tick                     tick_node;         /*!< node in softtimer Q   */
    struct list_head                    sem_node;          /*!< node in pending Q     */
    struct list_head                   *pending_resource;  /*!< pend node if any      */
#if CONFIG_MUTEX_EN
    struct list_head                    mutex_holded_head; /*!< remember all mutex    */
    int                                 priority;          /*!< normal priority       */
#endif
    struct list_head                    task_list_node;    /*!< node in task list     */
    int                                 option;            /*!< for the further       */
    int                                 current_priority;  /*!< running priority      */
    int                                 err;               /*!< errno used internal   */
#if CONFIG_TASK_TERMINATE_EN
    int                                 safe_count;        /*!< prevent task deletion */
#endif
    int                                 status;            /*!< task status           */
};


/**
 *  @brief semaphore definition
 *
 *  @sa SEMAPHORE_API
 */
struct rtk_semaphore
{
    union {
        unsigned int        count; /*!< counter when it is semB or sem C    */
        struct rtk_task     *owner; /*!< owner when it is mutex              */
    }u;
    struct list_head        pending_tasks;          /*!< the pending tasks link list head    */
    unsigned char           type;                   /*!< Specifies the type of semaphore     */
};

/**
 *  @brief mutex struct
 */
struct rtk_mutex
{
    struct rtk_semaphore s;
    struct list_head     sem_member_node; /*!< only used when it is mutex          */
    int                  mutex_recurse_count; /*!< only used when it is mutex          */
};


/**
 *  @brief struct rtk_msgq struct definition
 *
 *  @sa  MSGQ_API  
 */
struct rtk_msgq
{
    struct rtk_semaphore sem_rd;         /*!< read semaphore          */
    struct rtk_semaphore sem_wr;         /*!< write semaphore         */
    int         buff_size;      /*!< buffer size             */
    int         count;          /*!< max unit                */
    int         unit_size;      /*!< element size            */
    int         rd;             /*!< read pointor            */
    int         wr;             /*!< write pointor           */
    char       *buff;        /*!< dynamic allocated       */
};


/**
 *  @}
 */

/**
 *  @defgroup kernel_const_value const definition
 *  @{
 */
/**
 *  @defgroup task_status   task status definition
 *  @{
 */
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
 *  @defgroup struct rtk_semaphoreype    semaphore type
 *  @{
 */
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
#ifndef WAIT_FOREVER
#define WAIT_FOREVER    ((unsigned int)0xffffffff)
#endif
/**
 *  @}
 */


/**
 *  @defgroup API
 *  @{
 */

/**
 *  @defgroup TASK_API task API
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
 *      static TASK_INFO_DECL(info1, 1024);
 *      static TASK_INFO_DECL(info2, 1024);
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
/* #define TASK_INFO_DECL(info, stack_size) struct __taskinfo_##info {struct rtk_task tcb; char stack[stack_size]; }info */
#define TASK_INFO_DEF(info, stack_size) struct __taskinfo_##info {struct rtk_task tcb; char stack[stack_size]; }info

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
void rtk_init( void );

/**
 * @brief task control block init.
 *
 * @param task          pointor of task control block. You provide it.
 * @param name          name of the task. This string is copied to the top of the stack.
 * @param priority      task normal running priority.
 * @param stack_low     low address of the stack.
 * @param stack_high    high address of the stack.
 * @param pfunc         task entry function.
 * @param arg1          argument 1 to the task.
 * @param arg2          argument 2 to the task.
 *
 *  A task is initialized by this function but not be added to running queue.
 *  A task_starup(task) will add a initialized task to running queue.
 *  So you must use function task_startup() to start a initialized task.
 * @sa task_starup();
 */
struct rtk_task *task_init(struct rtk_task *task,
                          const char     *name,
                          int             priority, /* priority of new task */
                          int             option, /* task option word */
                          char *          stack_low,
                          char *          stack_high,
                          void           *pfunc, /* entry point of new task */
                          void           *arg1, /* 1st of 10 req'd args to pass to entryPt */
                          void           *arg2);
    
/**
 *  @brief task start up.
 *
 *  add a task to running queue.
 */
int task_startup( struct rtk_task *task );


/**
 *  @brief starting up operating system.
 *
 *  calling this function will start the scheduler, and switch to
 *  the highest priority task created. So, task must be created and started up
 *  before calling this function.
 */
void rtk_startup( void );

/**
 *  @brief set task priority 
 *  @fn task_priority_set
 *  @param[in]  task            task control block pointor. If NULL, current task's
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
int task_priority_set( struct rtk_task *task, unsigned int priority );

int task_errno_set( struct rtk_task *task, int err );
int task_errno_get( struct rtk_task *task );


/**
 *  @brief stop a task and deinitialize.
 *
 *  the task may be running or pending.
 */
int task_terminate( struct rtk_task *task );

/**
 *  @brief task delay
 *
 *  delay tick.
 */
void task_delay( int tick );

/**
 *  \brief system tick get
 */
unsigned int tick_get( void );

int task_safe( void );
int task_unsafe( void );

/**
 *  @brief current task name
 */
#define CURRENT_TASK_NAME() (task_self()->name+0)

/**
 *  @brief current task priority
 */
#define CURRENT_TASK_PRIORITY() (task_self()->priority+0)

/**
 *  @brief current task errno
 */
#define CURRENT_TASK_ERR() (task_self()->err+0)

/**
 *  @}
 */

/**
 *  @defgroup SEMAPHORE_API semaphore API
 *  @{
 */

/**
 *  @brief semaphore declaration macro
 *
 *  @par example  
 *  @code
 *  struct rtk_semaphore semb;
 *  struct rtk_semaphore semc;
 *  struct rtk_mutex mutex;
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
#define SEM_DECL(sem) struct rtk_semaphore sem
#define SEM_DEF( sem, t, init)                 \
    struct rtk_semaphore sem; struct rtk_semaphore sem = {  \
        {init},                                 \
        LIST_HEAD_INIT(sem.pending_tasks),      \
        t,                                      \
    }

/**
 *  @brief semaphore binary declaration macro.
 */
#define SEMB_DECL(name)  SEM_DECL(name)
#define SEMB_DEF(name, init_value)   SEM_DECL(name, SEM_TYPE_BINARY, init_value)

/**
 *  @brief semaphore counter declaration macro.
 */
#define SEMC_DECL(name)   SEM_DECL(name)
#define SEMC_DEF(name, init_value)   SEM_DECL(name, SEM_TYPE_COUNTER, init_value)

/**
 *  @brief mutex declaration macro.
 */
#define MUTEX_DECL(var) struct rtk_mutex var
#define MUTEX_DEF(var)                                            \
    struct rtk_mutex var={                                        \
        {                                                         \
            {0}, LIST_HEAD_INIT((var).s.pending_tasks),           \
            SEM_TYPE_MUTEX,                                       \
        },                                                        \
        LIST_HEAD_INIT((var).sem_member_node),                    \
        0,                                                        \
    }
    
/**
 *  \brief Initialize a counter semaphore.
 *
 *  \param[in]  semid       pointer
 *  \param[in]  initcount   Initializer: 0 or 1.
 *  \return     0           always successfully.
 *  \attention  parameter is not checked. You should check it by yourself.
 */
int  semc_init( struct rtk_semaphore *semid, int InitCount );

/**
 *  @brief semaphore counter deinitialize
 */
int  semc_terminate( struct rtk_semaphore*semid );

/**
 *  \brief Initialize a binary semaphore.
 *
 *  \param[in]  semid       pointer
 *  \param[in]  initcount   Initializer: 0 or 1.
 *  \return     0           always successfully.
 *  \attention  parameter is not checked. You should check it by yourself.
 */
int  semb_init( struct rtk_semaphore *semid, int InitCount );

/**
 *  \brief make a semaphore binary invalidate.
 *
 *  \param[in] semid    pointer
 *  \return 0           successfully.
 *  \return -EPERM      Permission Denied.
 *
 *  make the semaphore invalidate. This function will wake up all
 *  the pending tasks with parameter -ENXIO, and the pending task will
 *  get an error -ENXIO returning from semb_take().
 *
 *  \sa semb_init()
 */
int  semb_terminate( struct rtk_semaphore*semid );

/**
 *  \brief aquire a semaphore counter.
 *  \param[in] semid    semaphore pointer
 *  \param[in] tick     max waiting time in systerm tick.
 *                      if tick == 0, it will return immedately without block.
 *                      if tick == -1, it will wait forever.
 *  \return     0       successfully.
 *  \return     -EPERM  permission denied.
 *  \return     -EINVAL Invalid argument
 *  \return     -ETIME  time out.
 *  \return     -ENXIO  semaphore is terminated by other task or interrupt service routine.
 *  \return     -EAGAIN Try again. Only when tick==0 and semaphore is not available.
 */
int  semc_take( struct rtk_semaphore *semid, unsigned int tick );

/**
 *  \brief release a counter semaphore.
 *  \param[in] semid    pointer
 *  \return     0       successfully.
 *  \return     -EPERM  permission denied.
 *  \return     -ENOSPC no space to perform give operation.
 *  \return     -EINVAL Invalid argument
 *  \note               can be used in interrupt service.
 */
int  semc_give( struct rtk_semaphore *semid );


/**
 *  \brief aquire a semaphore binary.
 *  \param[in] semid    semaphore pointer
 *  \param[in] tick     max waiting time in systerm tick.
 *                      if tick == 0, it will return immedately without block.
 *                      if tick == -1, it will wait forever.
 *  \return     0       successfully.
 *  \return     -EPERM  permission denied.
 *  \return     -EINVAL Invalid argument
 *  \return     -ETIME  time out.
 *  \return     -ENXIO  semaphore is terminated by other task or interrupt service routine.
 *  \return     -EAGAIN Try again. Only when tick==0 and semaphore is not available.
 *
 */
int  semb_take( struct rtk_semaphore *semid, unsigned int tick );

/**
 *  \brief release a binary semaphore.
 *  \param[in] semid    pointer
 *  \return     0       successfully.
 *  \return     -EPERM  permission denied.
 *  \return     -EINVAL Invalid argument
 *  \note               can be used in interrupt service.
 */
int  semb_give( struct rtk_semaphore *semid );

/**
 *  \brief initialize a mutex.
 *  
 *  \param[in]  semid       pointer
 *  \return     0           always successfully.
 *  \attention  parameter is not checked. You should check it yourself.
 *  \sa struct rtk_mutexerminate()
 */
int  mutex_init( struct rtk_mutex *semid );

/**
 *  \brief aquire a mutex lock.
 *  \param[in] semid    mutex pointer
 *  \param[in] tick     max waiting time in systerm tick.
 *                      if tick == 0, it will return immedately without block.
 *                      if tick == -1, it will wait forever.
 *  \return     0       successfully.
 *  \return     -EPERM  permission denied.
 *  \return     -EINVAL Invalid argument
 *  \return     -ETIME  time out.
 *  \return     -EDEADLK Deadlock condition detected.
 *  \return     -ENXIO  mutex is terminated by other task or interrupt service routine.
 *  \return     -EAGAIN Try again. Only when tick==0 and mutex is not available.
 *
 *  \attention          cannot be used in interrupt service.
 */
int  mutex_lock( struct rtk_mutex *semid, unsigned int tick );

/**
 *  \brief release a mutex lock.
 *  \param[in] semid    mutex pointer
 *  \return     0       successfully.
 *  \return     -EPERM  permission denied.
 *                      The mutex's ownership is not current task. Or
 *                      used in interrupt context.
 *  \return     -EINVAL Invalid argument
 *  \attention          cannot be used in interrupt service.
 */
int  mutex_unlock( struct rtk_mutex *semid );

/**
 *  \brief make a mutex invalidate.
 *
 *  \param[in] semid    mutex pointer
 *  \return 0       successfully.
 *  \return -EPERM  Permission Denied.
 *
 *  make the mutex invalidate. This function will wake up all
 *  the pending tasks with parameter -ENXIO, and the pending task will
 *  get an error -ENXIO returning from mutex_take().
 *
 *  \sa mutex_init()
 */
int mutex_terminate( struct rtk_mutex *mutex );
/**
 *  @}
 */


/**
 *  \defgroup MSGQ_API message queue API
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
 *      return msgq_send( (struct rtk_msgq*)&mymsgq, buff, sizeof(int), 1000 );
 *  }
 *  int func_recieve( int *buff )
 *  {
 *      return msgq_recieve( (struct rtk_msgq*)mymsgq, buff, sizeof(int), -1, 0);
 *  }
 *  @endcode
 */

#define MSGQ_DECL_INIT(zone, name, unitsize, cnt)                             \
    zone char __msgqbuff##name[(unitsize)*(cnt)];                  \
    zone struct rtk_msgq name; struct rtk_msgq name = {                                           \
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
 *      return msgq_recieve( (struct rtk_msgq*)mymsgq, buff, sizeof(int), -1, 0);
 *  }
 *  @endcode
 */
#define MSGQ_DECL(name, buffersize ) 
#define MSGQ_DEF_NO_INIT(name, buffersize )                             \
    struct __msgq_def_##name {                                          \
        char   buffer[buffersize];                                      \
        struct rtk_msgq msgq;                                           \
    } name;
#define MSGQ_DO_INIT(name, unitsize)  msgq_init( &name.msgq, msgq.buffer, sizeof(msgq.buffer), unitsize)

/**
 *  \brief Initialize a message queue.
 *
 *  \param[in]  pmsgq       pointer
 *  \param[in]  buff        buffer pointer.
 *  \param[in]  buffer_size buffer size.
 *  \param[in]  unit_size   element size.
 *  \return     pmsgq       successfully.
 *  \return     NULL        Invalid argument.
 *  \attention  parameter is not checked. You should check it by yourself.
 *
 *  @par example  
 *  @code
 *      #define COUNT 10
 *      #define M sizeof(struct _yourbase_type)*COUNT
 *      struct _yourbase_type {
 *          int a;int b;
 *      };
 *      char buffer[M];
 *      struct rtk_msgq msgq_var;
 *      void func_init( void ){
 *          msgq_init( &msgq_var, buffer, sizeof(buffer), (struct _yourbase_type));
 *      }
 *  @endcode
 */
struct rtk_msgq *msgq_init( struct rtk_msgq *pmsgq, void *buff, int buffer_size, int unit_size );

/**
 *  \brief make a msgq invalidate.
 *
 *  \param[in] pmsgq    pointer
 *  \return 0           successfully.
 *  \return -EPERM      Permission Denied.
 *  
 *  make the msgq invalidate. This function will wake up all
 *  the pending tasks with parameter -ENXIO, and the pending task will
 *  get an error -ENXIO returning from msgq_receive()/msgq_send().
 *  
 *  \sa msgq_init()
 */
int msgq_terminate( struct rtk_msgq *pmsgq );


/**
 *  @brief receive msg from a msgQ
 *  @param pmsgq     a pointer to the msgQ.(the return value of function msgq_create)
 *  @param buff      the memory to store the msg. It can be NULL. if
 *                   it is NULL, it just remove one message from the head.
 *  @param buff_size the buffer size.
 *  @param tick      the max time to wait if there is no message.
 *                   if pass -1 to this, it will wait forever.
 *  @return -1       error, please check errno. if errno == ETIME, it means Timer expired,
 *                   if errno == ENOMEM, it mean buffer_size if not enough.
 *  @return 0        receive successfully.
 */
int msgq_receive( struct rtk_msgq *pmsgq, void *buff, int buff_size, int tick );

/**
 *  @brief send message to a the message Q.
 *  @param pmsgq     a pointer to the msgQ.
 *  @param buff      the message to be sent.
 *  @prarm size      the size of the message to be sent in bytes.
 *  @param tick      if the msgQ is not full, this function will return immedately, else it
 *                   will block some tick. Set it to -1 if you want to wait forever.
 *  @return 0        successfully.
 *  @return -EINVAL  Invalid argument.
 *  @return -ENODATA size is 0.
 *  @return -ETIME   time expired.
 */
int msgq_send( struct rtk_msgq *pmsgq, const void *buff, int size, int tick );

/**
 *  @brief reset a message queue.
 *
 *  make the specified message queue write counter full,
 *  and read counter empty.
 *  This function will wake up the writer.
 */
int msgq_clear( struct rtk_msgq *pmsgq );

/**
 *  @}
 */

/**
 *  @defgroup OTHER_API other API
 *  @{
 */

void arch_interrupt_enable( int old );
int  arch_interrupt_disable( void );
void enter_int_context( void );
void exit_int_context( void );

extern int rtk_is_int_context;
extern void schedule(void);
#define IS_INT_CONTEXT()        (rtk_is_int_context>0)
#define ENTER_INT_CONTEXT()     enter_int_context()
#define EXIT_INT_CONTEXT()      exit_int_context()


#if CONFIG_TICK_DOWN_COUNTER_EN>0
void rtk_tick_down_counter_init(struct rtk_tick *_this);
int rtk_tick_down_counter_set_func( struct rtk_tick *_this, void (*func)(void*), void*arg );
void rtk_tick_down_counter_start( struct rtk_tick *_this, unsigned int tick );
void rtk_tick_down_counter_stop ( struct rtk_tick *_this );
#endif
/**
 * @brief task list head
 *
 * internal used currently
 */
/**
 * @brief pontor of current task control bock.
 *
 * @ATTENTION  you should NEVER modify it. 
 */
extern struct rtk_task *task_self(void);

/**
 *  @brief     task_yield
 *
 *
 *  note: idle task cannot call this function
 *  @param  N/A
 *  @return N/A. yield cpu.
 */
void task_yield( void );

/**
 *  @}
 */

/**
 *  @brief user never use this.
 *
 *  this is private structure.
 */
struct __priority_q_bitmap_head
{
    struct rtk_private_priority_q_node *phighest_node;
    unsigned int                        bitmap_group;
    unsigned int                        max_priority;
    struct list_head                   *tasks;
    uint32_t                            bitmap_tasks[1];
};

/**
 *  @brief rtk max priority def
 *
 *  define rtk max priroty.
 *  _max_priority can be 8~1023
 */
#define RTK_MAX_PRIORITY_DEF(_max_priority)                             \
    struct __priority_q_bitmap_head_def                                 \
    {                                                                   \
        struct rtk_private_priority_q_node *phighest_node;              \
        unsigned int                        bitmap_group;               \
        unsigned int                        max_priority;               \
        struct list_head                   *tasks;                      \
        uint32_t                            bitmap_tasks[((_max_priority+1)&(32-1)) \
                                                         ?((_max_priority+1)/32+1) \
                                                         :((_max_priority+1)/32)]; \
        struct list_head                    _tasks[_max_priority+1];    \
    };                                                                  \
    static struct __priority_q_bitmap_head_def __rtk_readyq;            \
    static struct __priority_q_bitmap_head_def __rtk_readyq = {0,0,_max_priority,__rtk_readyq._tasks,}; \
    struct __priority_q_bitmap_head *g_rtk_ready_q          = (struct __priority_q_bitmap_head*)&__rtk_readyq
    
unsigned int rtk_max_priority( void );

/**
 *  @}
 */

/**
 *  @}
 */

/**
 *  @}
 */

    

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */


#endif

