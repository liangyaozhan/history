/* Last modified Time-stamp: <2014-08-10 12:55:05, by lyzh>
 * 
 * Copyright (C) 2012 liangyaozhan <ivws02@126.com>
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
 *
 *  task status:
 *
 *  READY 0:
 *  only PENDING 1:
 *  only DELAY   2:
 *  PENDING&DELAY 3
 *  TASK_DEAD: 0xF0
 *  TASK_PREPARED: 0x5c
 *  
 */
#include "list.h"
#include "rtk.h"
#include "err.h"



#define PRIO_NODE_TO_PTCB(pdn)        list_entry(pdn, struct rtk_task, prio_node)
#define TICK_NODE_TO_PTCB(pdn)        list_entry(pdn, struct rtk_task, tick_node)
#define PEND_NODE_TO_PTCB(pdn)        list_entry(pdn, struct rtk_task, sem_node)
#define READY_Q_REMOVE( task )          priority_q_remove( &(task)->prio_node )
#define READY_Q_PUT( task, key)         priority_q_put(&(task)->prio_node, key)
#define PLIST_PTR_TO_SEMID( ptr )       list_entry( (ptr), struct rtk_semaphore, pending_tasks )
#define SEM_MEMBER_PTR_TO_SEMID( ptr )  list_entry( (ptr), struct rtk_mutex, sem_member_node )
#define __task_detach_delay_counter(task) __rtk_tick_down_counter_remove( &task->tick_node );
#define highest_task_get()             PRIO_NODE_TO_PTCB( the_readyq()->phighest_node )
#define the_readyq()                  (g_rtk_ready_q)
#define MAX_PRIORITY                    rtk_max_priority()
    
#ifndef NULL
#define NULL               ((void*)0)
#endif
#define LIST_HEAD_FIRST(l) ((l)->next)


/**
 *  @brief optimize macro
 */
#define likely(x)    __builtin_expect(!!(x), 1)  /*!< likely optimize macro      */
#define unlikely(x)  __builtin_expect(!!(x), 0)  /*!< unlikely optimize macro    */

#undef int_min
#define int_min(a, b) ((a)>(b)?(b):(a))

typedef struct __priority_q_bitmap_head priority_q_bitmap_head_t;
typedef struct rtk_private_priority_q_node pqn_t;
extern struct __priority_q_bitmap_head *g_rtk_ready_q;

unsigned int rtk_max_priority( void )
{
    return the_readyq()->max_priority;
}

/*********************************************************************************************************
 **  globle var
 ********************************************************************************************************/
static struct list_head          g_softtime_head;
volatile unsigned long           g_systick;
static struct rtk_task           *rtk_task_current;
int                              rtk_is_int_context;
struct list_head                 g_systerm_tasks_head;

/**
 *  @brief Find First bit Set
 */
extern int rtk_ffs( register unsigned int q );


void *memcpy(void*,const void*,int);
static inline void    __sem_add_pending_task( struct rtk_semaphore *semid, struct rtk_task *task );
static inline void    __task_detach_pending_sem( struct rtk_task *task );
static inline int     __sem_pend_list_priority_get ( struct rtk_semaphore *semid );
static void           task_delay_timeout( struct rtk_tick *_this );
static void           priority_q_init( void );
static int            priority_q_put( pqn_t *_this, int key );
static int            priority_q_remove( pqn_t *_this );
static void           __rtk_tick_down_counter_set_func( struct rtk_tick *_this, void (*func)(void *), void *);
static void           __rtk_tick_down_counter_init(struct rtk_tick *_this);
static void           __rtk_tick_down_counter_add(struct rtk_tick *_this, unsigned int tick);
static void           __rtk_tick_down_counter_remove ( struct rtk_tick *_this );
void                  rtk_tick_down_counter_announce( void );
#if CONFIG_MUTEX_EN
static void           __restore_current_task_priority( struct rtk_mutex *semid );
static int            __mutex_owner_set( struct rtk_mutex *semid, struct rtk_task *taskToAdd );
static inline int     __task_mutex_hold_list_priority_get ( struct rtk_task *task );
static void           __task_pend_internal( struct rtk_task *task, unsigned int tick,
                                            int ( *task_wakeup)( struct rtk_task*, void * ),
                                            void *arg );
static void           __release_one_mutex( struct rtk_mutex *semid, int err_code );
static int            __mutex_raise_owner_priority( struct rtk_mutex *semid, int priority );
static int            __mutex_add_pending_task_trig( struct rtk_semaphore *semid, struct rtk_task *task );
#endif
static struct rtk_task*highest_task_loop_get( void );
extern void           arch_context_switch(void **fromsp, void **tosp);
extern void           arch_context_switch_interrupt(void **fromsp, void **tosp);
void                  arch_context_switch_to(void **sp);
static void           schedule_internel( void );
extern unsigned char *arch_stack_init(void *tentry, void *parameter1, void *parameter2,
                      char *stack_low, char *stack_high, void *texit);
static int            __sem_wakeup_penders( struct rtk_semaphore *semid, int err_code, int count );
#if CONFIG_DEAD_LOCK_DETECT_EN
static int            __mutex_dead_lock_detected( struct rtk_mutex * semid );
#endif
#if CONFIG_DEAD_LOCK_SHOW_EN
void                  __mutex_dead_lock_show( struct rtk_mutex *mutex );
#endif
static void           task_exit( void );


struct rtk_task *task_self(void)
{
    return rtk_task_current;
}

inline static struct rtk_task *rtk_set_self(struct rtk_task *p)
{
    rtk_task_current = p;
    return p;
}

static
void priority_q_init( void )
{
    priority_q_bitmap_head_t *pqriHead = the_readyq();
    unsigned int              m        = rtk_max_priority();
    unsigned int              i;
    int                       n;

    pqriHead->phighest_node = NULL;

    pqriHead->bitmap_group = 0;
    n = ((m+1)&(32-1))?((m+1)/32+1):((m+1)/32);

    for (i = 0; i<n; i++) {
        pqriHead->bitmap_tasks[i] = 0;
    }

    for (i = 0; i<=m; i++) {
        INIT_LIST_HEAD( &pqriHead->tasks[i] );
    }
}

static inline
void priority_q_bitmap_set ( priority_q_bitmap_head_t *pqriHead, int priority )
{
    register int grp = priority>>5;
    
    pqriHead->bitmap_tasks[ grp ] |= 1 << ( 0x1f & priority);
    pqriHead->bitmap_group                |= 1 << grp;
}

static inline
void priority_q_bitmap_clear( priority_q_bitmap_head_t *pqriHead, int priority )
{
    int group                         = priority>>5;
    pqriHead->bitmap_tasks[ group  ] &= ~(1 << ( 0x1f & priority));
    
    if ( unlikely(0 == pqriHead->bitmap_tasks[ group  ]) ) {
        pqriHead->bitmap_group &= ~(1 << group);
    }
}

static inline
pqn_t *priority_q_highest_get( priority_q_bitmap_head_t *pqHead )
{
    int      index;
    int      i;
    
    if ( unlikely(!pqHead->bitmap_group) ) {
        return NULL;
    }
    
    i = rtk_ffs( pqHead->bitmap_group ) - 1;
    index = rtk_ffs( pqHead->bitmap_tasks[i] ) + (i << 5) - 1;
    return list_entry( LIST_HEAD_FIRST( &pqHead->tasks[ index ] ), pqn_t, node);
}

int priority_q_put( pqn_t *_this, int key )
{
    register priority_q_bitmap_head_t *pqHead = the_readyq();

    /* cannot put more than once time, but, if key not the same, we change it.  */
    if ( unlikely(!list_empty(&_this->node)) ) {
        if ( likely(_this->key == key) ) {
            return -1;
        } else {
            priority_q_remove( _this );
        }
    }
    _this->key = key;
    priority_q_bitmap_set( pqHead, key );
    list_add_tail( &_this->node, &pqHead->tasks[key] );

    /*
     *  set high node
     */
    if ( unlikely(NULL == pqHead->phighest_node) || (key < pqHead->phighest_node->key) ) {
        pqHead->phighest_node = _this;
    }
    return 0;
}

int priority_q_remove( pqn_t *_this )
{
    register priority_q_bitmap_head_t *pqHead = the_readyq();
    int key;
    
    key = _this->key;
    list_del_init( &_this->node );
    if ( list_empty( &pqHead->tasks[key] ) ) {
        priority_q_bitmap_clear( pqHead, key );
    }
    if ( pqHead->phighest_node == _this  ) {
        pqHead->phighest_node = priority_q_highest_get( pqHead );
    }
    return 0;
}

void task_delay( int tick )
{
    int old;
    int last;
    struct rtk_task *task_current = task_self();

    old = arch_interrupt_disable();
    READY_Q_REMOVE( task_current );
    last = task_current->err;
    task_current->status = TASK_DELAY;
    __rtk_tick_down_counter_add( &task_current->tick_node, tick );
    schedule_internel();
    task_current->err = last;
    task_current->status = TASK_READY;
    arch_interrupt_enable(old);
}


/**
 *  @brief     task_yield
 *
 *
 *  note: idle task cannot call this function
 *  @param  N/A
 *  @return N/A. yield cpu.
 */
void task_yield( void )
{
    int              old;
    struct rtk_task *task_last;
    struct rtk_task *task_current = task_self();

    old = arch_interrupt_disable();
    READY_Q_REMOVE( task_current );
    task_last = task_current;
    task_current = rtk_set_self( highest_task_get() );
    READY_Q_PUT(task_last, task_last->priority );
    if ( IS_INT_CONTEXT() ) {
        arch_context_switch_interrupt( &task_last->sp, &task_current->sp );
    } else {
        arch_context_switch( &task_last->sp, &task_current->sp );
    }
    arch_interrupt_enable(old);
}


static void __task_pend_internal( struct rtk_task *task, unsigned int tick,
                                  int ( *task_wakeup)( struct rtk_task*, void * ),
                                  void *arg )
{
    int status = 0;
    if ( tick != WAIT_FOREVER ) {
        status = TASK_DELAY;
        __rtk_tick_down_counter_add( &task->tick_node, tick );
    }

    task->err    = 0;
    status = status | TASK_PENDING;
    do {
        task->status = status;
        READY_Q_REMOVE( task );
        schedule_internel();
        if ( (*task_wakeup)( task, arg ) ) {
            break;
        }
    } while ( !task->err );
    
    /*
     *  since the task is wake up, must remove from timer.
     */
    task->status = TASK_READY;
    __task_detach_pending_sem( task );
    __task_detach_delay_counter( task );
}

static
void task_delay_timeout( struct rtk_tick *_this )
{
    struct rtk_task *p;
    
    p = TICK_NODE_TO_PTCB( _this );
    p->err = ETIME;
    READY_Q_PUT( p, p->current_priority );
    p->status = TASK_READY;
}

static
struct rtk_task *highest_task_loop_get( void )
{
    struct rtk_task *task_current = task_self();

    if ( task_current->status == TASK_READY ) {
        READY_Q_REMOVE( task_current );
        READY_Q_PUT( task_current, task_current->current_priority );
    }
    return highest_task_get();
}

void rtk_startup( void )
{
    struct rtk_task *task = highest_task_get();
    rtk_set_self( task );
    arch_context_switch_to(&task->sp);
}


static void __rtk_tick_down_counter_init(struct rtk_tick *_this)
{
    INIT_LIST_HEAD( &_this->node );
}

void __rtk_tick_down_counter_add( struct rtk_tick *_this, unsigned int tick )
{
    struct list_head *p;
    struct rtk_tick  *p_tick = 0;
    
    list_for_each( p, &g_softtime_head ) {
        p_tick = list_entry(p, struct rtk_tick, node);
        if (tick > p_tick->tick ) {
            tick -= p_tick->tick;
        } else {
            break;
        }
    }
    _this->tick = tick;

    list_add_tail( &_this->node, p);
    if (p != &g_softtime_head ) {
        p_tick->tick -= tick;
    }
}

void __rtk_tick_down_counter_remove ( struct rtk_tick *_this )
{
    struct rtk_tick *p_tick;

    if ( list_empty(&_this->node) ) {
        return ;
    }

    if ( LIST_HEAD_FIRST( &_this->node ) != &g_softtime_head ) {
        p_tick = list_entry( LIST_HEAD_FIRST( &_this->node ), struct rtk_tick, node);
        p_tick->tick += _this->tick;
    }
    list_del_init( &_this->node );
}

void __rtk_tick_down_counter_set_func( struct rtk_tick *_this, void (*func)(void*), void *arg )
{
    _this->timeout_callback = func;
    _this->arg              = arg;
}

/**
 *  \brief soft timer announce.
 *
 *  systerm tick is provided by calling this function.
 *
 *  \sa ENTER_INT_CONTEXT(), EXIT_INT_CONTEXT().
 */
void rtk_tick_down_counter_announce( void )
{
    int old = arch_interrupt_disable();
    
    g_systick++;
    
    if ( !list_empty( &g_softtime_head ) ) {
        struct list_head *p;
        struct list_head *pNext;
        struct rtk_tick *_this;
        
        p = LIST_HEAD_FIRST( &g_softtime_head );
        _this = list_entry(p, struct rtk_tick, node);

        /*
         * in case of 'task_delay(0)'
         */
        if ( likely(_this->tick) ) {
            _this->tick--;
        }
        
        for (; !list_empty(&g_softtime_head); ) {
            pNext = LIST_HEAD_FIRST(p);
            _this = list_entry(p, struct rtk_tick, node);
            if ( _this->tick == 0) {
                list_del_init( &_this->node );
                if ( _this->timeout_callback ) {
                    (*_this->timeout_callback)( _this->arg );
                }
            } else {
                goto DoneOK;
            }
            p = pNext;
        }
    }
    
  DoneOK:
    arch_interrupt_enable(old);
}

static
void __sem_init_common( struct rtk_semaphore *semid )
{
    semid->u.count               = 0;
    INIT_LIST_HEAD( &semid->pending_tasks );
}

static
int __sem_terminate( struct rtk_semaphore *semid )
{
    int               old;
    int               happen = 0;

    old = arch_interrupt_disable();
    semid->type = SEM_TYPE_NULL;
    happen = __sem_wakeup_penders(semid, ENXIO, -1 );
    if ( happen ) {
        schedule_internel();
    }
    arch_interrupt_enable(old);

    return happen;
}

/**
 * \addtogroup SEMAPHORE_API    semaphore API
 * @{
 */
#if CONFIG_SEMC_EN
/**
 *  \brief Initialize a counter semaphore.
 *
 *  \param[in]  semid       pointer
 *  \param[in]  initcount   Initializer: 0 or 1.
 *  \return     0           always successfully.
 *  \attention  parameter is not checked. You should check it by yourself.
 */
int semc_init( struct rtk_semaphore *semid, int InitCount )
{
    __sem_init_common( semid );
    semid->u.count = InitCount;
    semid->type  = SEM_TYPE_COUNTER;
    return 0;
}

static int __sem_task_wakeup( struct rtk_task *task, void *arg )
{
    struct rtk_semaphore *semid = ( struct rtk_semaphore *)arg;
    
    if ( (task->err==0||task->err==ETIME) && semid->u.count )
    {
        semid->u.count--;
        task->err = 0;
        return 1;
    }
    return 0;
}

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
int semc_take( struct rtk_semaphore *semid, unsigned int tick )
{
    int old;
    struct rtk_task *task_current = rtk_task_current;

#if KERNEL_ARG_CHECK_EN
    if ( unlikely(semid->type != SEM_TYPE_COUNTER) ) {
        return -1;
    }
#endif
    if ( unlikely(IS_INT_CONTEXT()) ) {
        tick = 0;
    }
    old = arch_interrupt_disable();
    if ( semid->u.count ) {
        --semid->u.count;
        arch_interrupt_enable(old );
        return 0;
    }
    if ( tick == 0 ) {
        arch_interrupt_enable(old );
        return -EAGAIN;
    }
    __sem_add_pending_task( semid, task_current );
    __task_pend_internal( task_current, tick, __sem_task_wakeup, semid );
    arch_interrupt_enable(old );
    return -task_current->err;
}

/**
 *  \brief release a counter semaphore.
 *  \param[in] semid    pointer
 *  \return     0       successfully.
 *  \return     -EPERM  permission denied.
 *  \return     -ENOSPC no space to perform give operation.
 *  \return     -EINVAL Invalid argument
 *  \note               can be used in interrupt service.
 */
int semc_give( struct rtk_semaphore *semid )
{
    int               old;

#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(semid->type != SEM_TYPE_COUNTER) ) {
        return -EINVAL;
    }
#endif
    old = arch_interrupt_disable();
    if ( ++semid->u.count == 0 ) {
        --semid->u.count;
        arch_interrupt_enable(old );
        return -ENOSPC;
    }
    if ( __sem_wakeup_penders( semid, 0, 1 ) && !IS_INT_CONTEXT() ) {
        schedule_internel();
    }
    arch_interrupt_enable(old );
    return 0;
}

/**
 *  \brief reset a semaphore counter.
 *
 *  \sa semc_init(), semc_take(), semc_give(), semc_terminate()
 */
int semc_clear( struct rtk_semaphore *semid )
{
    int semb_clear( struct rtk_semaphore *semid );
    return semb_clear(semid);
}

/**
 *  \brief make a semaphore counter invalidate.
 *
 *  \param[in] semid    pointer
 *  \return 0           successfully.
 *  \return -EPERM      Permission Denied.
 *
 *  make the semaphore invalidate. This function will wake up all
 *  the pending tasks with parameter -ENXIO, and the pending task will
 *  get an error -ENXIO returning from semc_take().
 *
 *  \sa semc_init()
 */
int semc_terminate( struct rtk_semaphore *semid )
{
    return __sem_terminate(semid);
}

#endif /* CONFIG_SEMC_EN */

#if CONFIG_SEMB_EN
/**
 *  \brief Initialize a binary semaphore.
 *
 *  \param[in]  semid       pointer
 *  \param[in]  initcount   Initializer: 0 or 1.
 *  \return     0           always successfully.
 *  \attention  parameter is not checked. You should check it by yourself.
 */
int semb_init( struct rtk_semaphore *semid, int initcount )
{
    __sem_init_common( semid );
    semid->u.count = !!initcount;
    semid->type    = SEM_TYPE_BINARY;
    return 0;
}

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
int semb_take( struct rtk_semaphore *semid, unsigned int tick )
{
    int old;
    struct rtk_task *task_current = rtk_task_current;

#ifndef KERNEL_ARG_CHECK_EN
    if ( semid->type != SEM_TYPE_BINARY ) {
        return -EINVAL;
    }
#endif
    if ( unlikely(IS_INT_CONTEXT()) ) {
        tick = 0;
    }
    old = arch_interrupt_disable();
    if ( semid->u.count ) {
        semid->u.count = 0;
        arch_interrupt_enable(old );
        return 0;
    }
    if ( tick == 0 ) {
        arch_interrupt_enable(old );
        return -EAGAIN;
    }
    __sem_add_pending_task( semid, task_current );
    __task_pend_internal( task_current, tick, __sem_task_wakeup, semid );
    arch_interrupt_enable( old );
    return -task_current->err;
}

/**
 *  \brief reset a semaphore binary.
 *
 *  \sa semb_init(), semb_take(), semb_give(), semb_terminate()
 */
int semb_clear( struct rtk_semaphore *semid )
{
    int old;
    
    old = arch_interrupt_disable();
    semid->u.count = 0;
    arch_interrupt_enable(old );
    return 0;
}

/**
 *  \brief release a binary semaphore.
 *  \param[in] semid    pointer
 *  \return     0       successfully.
 *  \return     -EPERM  permission denied.
 *  \return     -EINVAL Invalid argument
 *  \note               can be used in interrupt service.
 */
int semb_give( struct rtk_semaphore *semid )
{
    int old;

#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(semid->type != SEM_TYPE_BINARY) ) {
        return -EPERM;
    }
#endif

    old = arch_interrupt_disable();

    semid->u.count = 1;
    if ( __sem_wakeup_penders( semid, 0, 1 ) && !IS_INT_CONTEXT() ) {
        schedule_internel();
    }
    arch_interrupt_enable(old );
    return 0;
}

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
int semb_terminate( struct rtk_semaphore *semid )
{
    return __sem_terminate(semid);
}
#endif

#if CONFIG_MUTEX_EN
/**
 *  \brief initialize a mutex.
 *  
 *  \param[in]  semid       pointer
 *  \return     0           always successfully.
 *  \attention  parameter is not checked. You should check it yourself.
 *  \sa mutex_terminate()
 */
int mutex_init( struct rtk_mutex *semid )
{
    __sem_init_common( (struct rtk_semaphore*)semid );
    INIT_LIST_HEAD( &semid->sem_member_node );
    semid->mutex_recurse_count  = 0;
    semid->s.type               = SEM_TYPE_MUTEX;
    return 0;
}

static int __mutex_task_wakeup( struct rtk_task*task, void *arg )
{
    struct rtk_mutex *mutex = ( struct rtk_mutex* )arg;
    if (( task->err==0|| task->err==ETIME ) &&  mutex->s.u.owner == NULL ) {
        __mutex_owner_set( mutex, task );
        mutex->mutex_recurse_count = 1;
        task->err = 0;
        return 1;
    }
    return 0;
}

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
int mutex_lock( struct rtk_mutex *semid, unsigned int tick )
{
    int old;
    struct rtk_task *task_current = rtk_task_current;

#if KERNEL_ARG_CHECK_EN
    if ( unlikely(IS_INT_CONTEXT()) ) {
        return -EPERM;
    }
    if ( unlikely(semid->s.type != SEM_TYPE_MUTEX) ) {
        return -EINVAL;
    }
#endif
    old = arch_interrupt_disable();
    if ( semid->s.u.owner == NULL ) {
        __mutex_owner_set( semid, task_current );
        semid->mutex_recurse_count++;
        arch_interrupt_enable(old );
        return 0;
    } else if ( semid->s.u.owner == task_current ) {
        semid->mutex_recurse_count++;
        arch_interrupt_enable(old );
        return 0;
    }
#if CONFIG_DEAD_LOCK_DETECT_EN
    else if ( __mutex_dead_lock_detected( semid ) ) {
#ifdef DEAD_LOCK_HOOK
        DEAD_LOCK_HOOK(semid, semid->s.u.owner );
#endif
#if CONFIG_DEAD_LOCK_SHOW_EN
        __mutex_dead_lock_show( semid );
#endif
        return -EDEADLK;/* Deadlock condition */
    }
#endif
    if ( tick == 0 ) {
        arch_interrupt_enable(old );
        return -EAGAIN;
    }
    /*
     *  put tcb into pend list and inherit priority.
     */
    if ( __mutex_add_pending_task_trig( &semid->s, task_current ) ) {
        __mutex_raise_owner_priority( semid, task_current->current_priority );
    }
    __task_pend_internal( task_current, tick, __mutex_task_wakeup, semid );
    arch_interrupt_enable( old );
    return -task_current->err;
}
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
int mutex_unlock( struct rtk_mutex *semid )
{
    int old;
    struct rtk_task *task_current = task_self();
    
#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(semid->s.type != SEM_TYPE_MUTEX) ) {
        return -EINVAL;
    }
    if ( unlikely(IS_INT_CONTEXT()) ) {
        return -EPERM;
    }
#endif
    old = arch_interrupt_disable();
    if ( semid->s.u.owner != task_current ) {
        arch_interrupt_enable(old );
        return -EPERM;
    }
    if ( --semid->mutex_recurse_count ) {
        arch_interrupt_enable(old );
        return 0;
    }
    
    __release_one_mutex( semid, 0 );
    __restore_current_task_priority( semid );
    schedule_internel();
    
    arch_interrupt_enable(old );
    return 0;
}

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
int mutex_terminate( struct rtk_mutex *semid )
{
    int old;

    old = arch_interrupt_disable();
    __mutex_owner_set( semid, NULL );
    __sem_terminate( (struct rtk_semaphore*)semid );
    arch_interrupt_enable( old );
    return 0;
}
#if CONFIG_DEAD_LOCK_DETECT_EN
static int __mutex_dead_lock_detected( struct rtk_mutex *semid )
{
    struct rtk_task *powner;
    struct rtk_mutex *s = semid;
    struct rtk_task *task_current = task_self();
    
    powner = s->s.u.owner;
again:
    if ( powner->status & TASK_PENDING ) {
        s = (struct rtk_mutex *)PLIST_PTR_TO_SEMID( powner->pending_resource );
        if ( s->s.type == SEM_TYPE_MUTEX ) {
            powner = s->s.u.owner;
            if ( powner == task_current  )
                return 1;
            goto again;
        }
    }
    return 0;
}


#if CONFIG_DEAD_LOCK_SHOW_EN
void __mutex_dead_lock_show( struct rtk_mutex *mutex )
{
    struct rtk_task *powner;
    struct rtk_mutex *s = mutex;
    struct rtk_task *task_current = task_self();
    powner = s->s.u.owner;

    kprintf("Dead lock path:\n task %s pending on 0x%08X", task_current->name, mutex);
again:
    kprintf(", taken by %s", powner->name );
    if ( powner->status & TASK_PENDING ) {
        s = (struct rtk_mutex *)PLIST_PTR_TO_SEMID( powner->pending_resource );
        kprintf(", and pending on 0x%08X ", s );
        if ( s->s.type == SEM_TYPE_MUTEX ) {
            powner = s->s.u.owner;
            if ( powner == task_current ) {
                kprintf(", taken by %s\n", powner->name );
                return;
            }
            goto again;
        }
    }
    kprintf(".\n");
}
#endif
#endif
#endif /* CONFIG_MUTEX_EN */

/** @} */
#if CONFIG_MUTEX_EN
static
void __release_one_mutex( struct rtk_mutex *semid, int error_code )
{
    __sem_wakeup_penders( (struct rtk_semaphore*)semid, error_code, 1 );
    __mutex_owner_set( semid, NULL );
}
#endif /* CONFIG_MUTEX_EN */

static
int __sem_pend_list_priority_get ( struct rtk_semaphore *semid )
{
    struct rtk_task *task;
    
    if ( !list_empty(&semid->pending_tasks) ) {
        task = PEND_NODE_TO_PTCB( LIST_HEAD_FIRST(&semid->pending_tasks) );
        return task->current_priority;
    }
    return MAX_PRIORITY+1;
}

static
void __sem_add_pending_task( struct rtk_semaphore *semid, struct rtk_task *task )
{
    struct rtk_task  *task_i;
    struct list_head *p;
    
    list_for_each( p, &semid->pending_tasks) {
        task_i = PEND_NODE_TO_PTCB( p );
        if ( task_i->current_priority > task->current_priority ) {
            break;
        }
    }
    
    list_add_tail( &task->sem_node, p );
    task->pending_resource = &semid->pending_tasks;
}

static
void __task_detach_pending_sem( struct rtk_task *task )
{
    list_del_init( &task->sem_node );
    task->pending_resource = NULL;
}

static
int __sem_resort_pend_list_trig( struct rtk_semaphore *semid, struct rtk_task *task )
{
    int pri;

    __task_detach_pending_sem( task );
    pri = __sem_pend_list_priority_get(semid);
    __sem_add_pending_task( semid, task );
    return pri > __sem_pend_list_priority_get(semid);
}

static
int __sem_wakeup_penders( struct rtk_semaphore *semid, int err, int count )
{
    register int               n;
    register struct list_head *p;
    register struct list_head *save;
    register struct rtk_task   *taskwakeup;

    n = 0;
    list_for_each_safe( p, save, &semid->pending_tasks ) {
        taskwakeup = PEND_NODE_TO_PTCB( p );
        /*
         *  some one notice us error ocurse with sem, we must detach
         *  task from sem.
         */
        if ( err ) {                    /*!  really wake up the task */
            __task_detach_pending_sem( taskwakeup );
            __rtk_tick_down_counter_remove( &taskwakeup->tick_node );
            taskwakeup->status    = TASK_READY;
        }
        READY_Q_PUT( taskwakeup, taskwakeup->current_priority );
        taskwakeup->err       = err;
        if ( ++n == count ) {
            break;
        }
    }
    return n;
}

#if CONFIG_MUTEX_EN
static
int __task_mutex_hold_list_priority_get ( struct rtk_task *task )
{
    struct rtk_mutex *semid;

    if ( !list_empty(&task->mutex_holded_head) ) {
        semid = SEM_MEMBER_PTR_TO_SEMID( LIST_HEAD_FIRST(&task->mutex_holded_head) );
        return __sem_pend_list_priority_get((struct rtk_semaphore*)semid);
    }
    return MAX_PRIORITY+1;
}

static
int  __mutex_owner_set( struct rtk_mutex *semid, struct rtk_task *task )
{
    struct list_head *p;
    struct rtk_mutex *psem;
    int               pri;

    if ( NULL == task ) {
        semid->s.u.owner = NULL;
        list_del_init( &semid->sem_member_node );
        return 0;
    }

    semid->s.u.owner = task;

    pri = __sem_pend_list_priority_get((struct rtk_semaphore*)semid);
    list_for_each( p, &task->mutex_holded_head) {
        psem = SEM_MEMBER_PTR_TO_SEMID( p );
        if ( __sem_pend_list_priority_get( (struct rtk_semaphore*)psem ) > pri )  {
            break;
        }
    }

    list_add_tail( &semid->sem_member_node, p );
    return 0;
}

static
int __resort_hold_mutex_list_and_trig( struct rtk_mutex *semid, struct rtk_task *task )
{
    int pri;
    __mutex_owner_set(semid, NULL);
    pri = __task_mutex_hold_list_priority_get(task);
    __mutex_owner_set(semid, task);
    return pri > __task_mutex_hold_list_priority_get(task);
}


static
int __mutex_add_pending_task_trig( struct rtk_semaphore *semid, struct rtk_task *task )
{
    int pri;

    pri = __sem_pend_list_priority_get(semid);
    __sem_add_pending_task( semid, task );
    return pri > __sem_pend_list_priority_get(semid);
}

static
int __mutex_raise_owner_priority( struct rtk_mutex *semid, int priority )
{
    int ret = 0;
    struct rtk_task *powner;
    powner   = semid->s.u.owner;

again:
    if ( __resort_hold_mutex_list_and_trig(semid, powner) ) {
        if ( powner->current_priority > priority ) {
            powner->current_priority = priority;
            ret++;
            if ( powner->status == TASK_READY ) {
                READY_Q_REMOVE( powner );
                READY_Q_PUT( powner, priority );
            } else if ( powner->status & TASK_PENDING ) {
                semid   = (struct rtk_mutex*)PLIST_PTR_TO_SEMID( powner->pending_resource );
                if ( __sem_resort_pend_list_trig( (struct rtk_semaphore*)semid, powner ) &&
                     (semid->s.type == SEM_TYPE_MUTEX) && semid->s.u.owner ) {
                    /*
                     *  recursive call this function.
                     *  __mutex_raise_owner_priority( semid, priority );
                     */
                    powner = semid->s.u.owner;
                    goto again;
                }
            }
        }
    }
    return ret;
}

static
void __restore_current_task_priority ( struct rtk_mutex *semid )
{
    int priority;
    struct rtk_task *task_current = task_self();

    /*
     *  find the highest priority needed to setup,
     *  which is from the mutex of current task holded.
     *  it will be always the first one of MutexHeadList.
     */
    priority = __task_mutex_hold_list_priority_get(task_current);
    if ( priority > task_current->priority ) {
        priority = task_current->priority;
    }

    if ( unlikely(priority != task_current->current_priority )) {
        READY_Q_REMOVE( task_current );
        READY_Q_PUT( task_current, priority );
        task_current->current_priority = priority;
    }
}

#endif /* CONFIG_MUTEX_EN */

#if CONFIG_TASK_PRIORITY_SET_EN
/**
 *  \addtogroup TASK_API    task API
 *  @{
 *  
 *  @brief set task priority 
 *  @fn task_priority_set
 *  @param[in]  task            task control block pointer. If NULL, current task's
 *                              priority will be change.
 *  @param[in]  new_priority    new priority.
 *  @return     0               successfully.
 *  @return     -EINVAL         Invalid argument.
 *  @return     -EPERM          Permission denied. The task is not startup yet.
 *  basic rules:
 *      1. if task's priority changed, we must check if we need to do something with
 *         it's pending resource (only when it's status is pending).
 *      2. if task's priority goes down, we must check mutex list's priority.
 *         Maybe it's priority cannot go down right now.
 *
 *            P0                 P1                P2
 *             |                  |                 |
 *  0(high) ==============================================>> 256(low priority)
 *                     ^                  ^
 *                     |                  |
 *              current priority    normal priority
 */
int task_priority_set( struct rtk_task *task, unsigned int priority )
{
    int old;
    int ret = 0;
    int need = 0;/* need to call scheduler */

    if ( task == NULL ) {
        task = task_self();
    }
    if ( priority > MAX_PRIORITY ) {
        return -EINVAL;
    }
    old = arch_interrupt_disable();

    if ( TASK_PREPARED == task->status ||
         TASK_DEAD == task->status  ) {
        ret = -EPERM;
        goto done;
    }
#if CONFIG_MUTEX_EN
    if ( task->priority == priority ) {
        goto done;
    }
    
    task->priority = priority;
    if ( priority < task->current_priority ) {     /* priority goes up */
        task->current_priority = priority;
    } else if ( __task_mutex_hold_list_priority_get( task ) >= priority ) {
        task->current_priority = priority;/* priority can go down at the moment. */
    }
#else
    task->current_priority = priority;
#endif

    if ( task->status & TASK_PENDING ) {
        struct rtk_semaphore *semid;
        int          trig;
        semid   = PLIST_PTR_TO_SEMID( task->pending_resource );
        trig = __sem_resort_pend_list_trig( (struct rtk_semaphore*)semid, task );
#if CONFIG_MUTEX_EN
        if ( trig && semid->type == SEM_TYPE_MUTEX ) {
            need = __mutex_raise_owner_priority( (struct rtk_mutex*)semid, task->current_priority );
        }
#else
        (void)trig;
#endif
    }

    if ( (task->status == TASK_READY) &&
         (task->current_priority == priority) ) {
        READY_Q_REMOVE( task );
        READY_Q_PUT( task, task->current_priority );
        need = 1;
    }
    if ( need && !IS_INT_CONTEXT()) {
        schedule_internel();
    }
    
done:
    arch_interrupt_enable( old );
    return ret;
}
#endif

/** @} */

struct rtk_task *task_init(struct rtk_task *task, 
                          const char     *name,
                          int             priority, /* priority of new task */
                          int             option, /* task option word */
                          char *          stack_low,
                          char *          stack_high,
                          void           *pfunc, /* entry point of new task */
                          void           *arg1, /* 1st of 10 req'd args to pass to entryPt */
                          void           *arg2)
{
    task->current_priority = priority;
    task->status           = TASK_PREPARED;
    task->pending_resource = (void*)0;
    task->option           = option;
    task->stack_low        = stack_low;
    task->stack_high       = stack_high;
    task->name             = name;
#if CONFIG_MUTEX_EN
    task->priority         = priority;
#endif
#if CONFIG_TASK_TERMINATE_EN
    task->safe_count       = 0;
#endif

    task->sp = arch_stack_init( pfunc, arg1, arg2, stack_low, stack_high, task_exit );
    INIT_LIST_HEAD( &task->prio_node.node );
    INIT_LIST_HEAD( &task->sem_node );
#if CONFIG_MUTEX_EN
    INIT_LIST_HEAD( &task->mutex_holded_head );
#endif
    INIT_LIST_HEAD( &task->task_list_node );
    __rtk_tick_down_counter_init( &task->tick_node );
    __rtk_tick_down_counter_set_func( &task->tick_node,
                                      (void(*)(void*))task_delay_timeout,
                                      &task->tick_node);
    return task;
}

int task_startup( struct rtk_task *task )
{
    int old;
    int ret = 0;

    old = arch_interrupt_disable();
    if ( task->status != TASK_PREPARED ) {
        arch_interrupt_enable(old );
        return -EPERM;
    }


    list_add_tail( &task->task_list_node, &g_systerm_tasks_head );
    READY_Q_PUT( task, task->current_priority );
    task->status = TASK_READY;
    /*
     *  do not call scheduler while kernel is not running ( at startup point ).
     */
    if ( NULL != task_self() ) {
        schedule_internel();
        ret = 0;
    }

    arch_interrupt_enable(old );
    return ret;
}

#if CONFIG_TASK_TERMINATE_EN
int task_safe( void )
{
    ++task_self()->safe_count;
    return 0;
}
int task_unsafe( void )
{
    --task_self()->safe_count;
    return 0;
}
/**
 *  \brief stop a task.
 *
 *  \param[in]  task    task control block pointer.
 *                      If NULL, it will equal to rtk_task_current.
 *
 *  \return     0       successfully.
 *  \return     -EPERM  Permission denied:
 *                      It is protected by calling task_safe().
 */
int task_terminate( struct rtk_task *task )
{
    int old;
    int ret = 0;
#if CONFIG_MUTEX_EN
    struct list_head *save, *p;
#endif

    if ( task == NULL ) {
        task = task_self();
    }

    old = arch_interrupt_disable();
    if ( task->safe_count ) {
        ret = -EPERM;
        goto done;
    }

    /*
     *  remove delay node
     */
    if ( !list_empty( &(task->tick_node.node) )) {
        __rtk_tick_down_counter_remove( &task->tick_node );
    }
#if CONFIG_MUTEX_EN
    /*
     *  release all mutex.
     */
    list_for_each_safe(p, save, &task->mutex_holded_head){
        struct rtk_mutex *psemid;
        psemid = SEM_MEMBER_PTR_TO_SEMID( p );
        __release_one_mutex( psemid, ENXIO );
    }
#endif
#if CONFIG_SEMB_EN||CONFIG_SEMC_EN
    __task_detach_pending_sem(task);
#endif
    READY_Q_REMOVE( task );
    list_del_init( &task->task_list_node );
    task->status = TASK_DEAD;
    schedule_internel();

done:
    arch_interrupt_enable(old );
    return ret;
}
#endif

static
void schedule_internel( void )
{
    struct rtk_task *p;

    p = highest_task_loop_get();
    if ( p != rtk_task_current) {
        struct rtk_task *p_old;
        p_old = task_self();
        rtk_set_self(p);
        arch_context_switch( &p_old->sp, &p->sp);        
    }
}

void schedule( void )
{
    int    old;
    struct rtk_task *task_last;
    struct rtk_task *task_current = task_self();

    old = arch_interrupt_disable();
    task_last = task_current;
    task_current = rtk_set_self( highest_task_loop_get() );
    if ( task_current != task_last ) {
        if ( IS_INT_CONTEXT() ) {
            arch_context_switch_interrupt( &task_last->sp, &task_current->sp );
        } else {
            arch_context_switch( &task_last->sp, &task_current->sp );
        }
    }
    arch_interrupt_enable(old);
}

static
void task_exit( void )
{
    struct rtk_task *task;
    struct rtk_task *task_current = task_self();
#if CONFIG_MUTEX_EN
    struct list_head *save, *p;
#endif

    arch_interrupt_disable();
    task_current->status = TASK_DEAD;
    READY_Q_REMOVE( task_current );
    list_del_init( &task_current->task_list_node );

#if CONFIG_MUTEX_EN
    /*
     *  release all mutex.
     */
    list_for_each_safe(p, save, &task_current->mutex_holded_head){
        struct rtk_mutex *psemid;
        psemid = SEM_MEMBER_PTR_TO_SEMID( p );
        __release_one_mutex( psemid, ENXIO );
    }
#endif
    task = highest_task_get();
    rtk_set_self(  task );
    arch_context_switch_to(&task->sp);
}


static
void task_idle( void *arg )
{
#if CONFIG_TASK_TERMINATE_EN
    task_safe();
#endif
    while (1) {
#ifdef IDLE_TASK_HOOK
        IDLE_TASK_HOOK;
#endif
        schedule();
    }
}

void enter_int_context( void )
{
    int old;
    old = arch_interrupt_disable();
    ++rtk_is_int_context;
    arch_interrupt_enable( old );
}
void exit_int_context( void )
{
    int old;
    schedule();
    old = arch_interrupt_disable();
    --rtk_is_int_context;
    arch_interrupt_enable( old );
}


void rtk_init( void )
{
    static TASK_INFO_DEF( info1, IDLE_TASK_STACK_SIZE );
    
    INIT_LIST_HEAD( &g_systerm_tasks_head );
    INIT_LIST_HEAD(&g_softtime_head);
    priority_q_init();
    TASK_INIT( "idle", info1, MAX_PRIORITY, task_idle, 0,0 );
    TASK_STARTUP(info1);
    rtk_set_self( NULL );
}

unsigned int tick_get( void )
{
    return g_systick;
}

#if CONFIG_MSGQ_EN
/**
 *  \brief Initialize a msgq.
 *
 *  \param[in]  pmsgq       pointer
 *  \param[in]  buff        buffer pointer.
 *  \param[in]  buffer_size buffer size.
 *  \param[in]  unit_size   element size.
 *  \return     pmsgq       successfully.
 *  \return     NULL        Invalid argument.
 *  \attention  parameter is not checked. You should check it by yourself.
 */
struct rtk_msgq *msgq_init( struct rtk_msgq *pmsgq, void *buff, int buffer_size, int unit_size )
{
    int     count;

    count = buffer_size / unit_size;
    
    if ( buffer_size == 0 || count == 0 ) {
        return NULL;
    }

    pmsgq->buff_size = buffer_size;
    pmsgq->rd        = 0;
    pmsgq->wr        = 0;
    pmsgq->unit_size = unit_size;
    pmsgq->count     = count;
    pmsgq->buff      = buff;

    semc_init( &pmsgq->sem_rd,  0 );
    semc_init( &pmsgq->sem_wr,  count );

    return pmsgq;
}

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
int msgq_terminate( struct rtk_msgq *pmsgq )
{
    int old;

    old = arch_interrupt_disable();
    semc_terminate( &pmsgq->sem_rd );
    semc_terminate( &pmsgq->sem_wr );
    arch_interrupt_enable( old );
    return 0;
}


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
int msgq_receive( struct rtk_msgq *pmsgq, void *buff, int buff_size, int tick )
{
    int ret;
    int rd;
    int old;

    old = arch_interrupt_disable();
    ret = semc_take( &pmsgq->sem_rd, tick );
    if ( ret ) {
        arch_interrupt_enable(old);
        return ret;
    }
    rd = pmsgq->rd;
    
    /* pmsgq->rd = (pmsgq->rd + 1) % pmsgq->count; */
    if ( ++pmsgq->rd >= pmsgq->count  )
    {
        pmsgq->rd = 0;
    }

    if ( buff ) {
        memcpy( buff, pmsgq->buff + pmsgq->unit_size*rd, pmsgq->unit_size );
    }
    
    semc_give( &pmsgq->sem_wr );
    arch_interrupt_enable(old);

    return 0;
}
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
int msgq_send( struct rtk_msgq *pmsgq, const void *buff, int size, int tick )
{
    int ret;
    int next;
    int wr;
    int old;
    
    if ( NULL == buff ) {
        return -EINVAL;
    }

    if ( 0 == size ) {
        /*
         *  nothing to be sent
         */
        return - ENODATA;
    }

    old = arch_interrupt_disable();
    /*
     *  this function can be used in interrupt context.
     */
    ret = semc_take( &pmsgq->sem_wr, tick );
    if ( ret ) {
        arch_interrupt_enable(old);
        return ret;
    }
    next = (pmsgq->wr + 1) % pmsgq->count;
    wr = pmsgq->wr;
    pmsgq->wr = next;
    
    if ( buff ) {
        memcpy( pmsgq->buff + pmsgq->unit_size*wr, buff, int_min(pmsgq->unit_size, size) );
    }
    
    semc_give( &pmsgq->sem_rd );
    arch_interrupt_enable(old);

    return 0;
}

int msgq_clear( struct rtk_msgq *pmsgq )
{
    int old;
    int n;

    if ( NULL == pmsgq ) {
        return -1;
    }
    
    old = arch_interrupt_disable();
    pmsgq->sem_wr.u.count = pmsgq->count;
    pmsgq->sem_rd.u.count = 0;
    pmsgq->rd             = pmsgq->wr = 0;

    n = __sem_wakeup_penders(&pmsgq->sem_wr, 0, pmsgq->sem_wr.u.count ) ;
    
    if ( n && !IS_INT_CONTEXT() ) {
        schedule_internel();
    }
    arch_interrupt_enable(old);
    return 0;
}
#endif

#if CONFIG_TICK_DOWN_COUNTER_EN>0

void rtk_tick_down_counter_init(struct rtk_tick *_this)
{
    __rtk_tick_down_counter_init(_this);
}
    
int rtk_tick_down_counter_set_func( struct rtk_tick *_this,
                                    void (*func)(void*),
                                    void            *arg )
{
    int old;
    old = arch_interrupt_disable();
    __rtk_tick_down_counter_set_func(_this, func, arg );
    arch_interrupt_enable(old);
    return 0;
}

void rtk_tick_down_counter_start( struct rtk_tick *_this, unsigned int tick )
{
    int old;
    
    old = arch_interrupt_disable();
    __rtk_tick_down_counter_remove( _this );
    __rtk_tick_down_counter_add( _this, tick );
    arch_interrupt_enable( old );
}

void rtk_tick_down_counter_stop ( struct rtk_tick *_this )
{
    int old;
    old = arch_interrupt_disable();
    __rtk_tick_down_counter_remove( _this );
    arch_interrupt_enable( old );
}


#endif
