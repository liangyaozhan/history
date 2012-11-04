/* Last modified Time-stamp: <2012-11-04 12:58:58 Sunday by lyzh>
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
#include "list.h"
#include "rtk.h"
#include "err.h"

#define PRIO_NODE_TO_PTCB(pNode)        list_entry(pNode, tcb_t, prio_node)
#define TICK_NODE_TO_PTCB(pNode)        list_entry(pNode, tcb_t, tick_node)
#define PEND_NODE_TO_PTCB(pNode)        list_entry(pNode, tcb_t, sem_node)
#define READY_Q_REMOVE( ptcb )          priority_q_remove( &(ptcb)->prio_node )
#define READY_Q_PUT( ptcb, key)         priority_q_put(&(ptcb)->prio_node, key)
#define DELAY_Q_PUT(ptcb, tick)         softtimer_add( &(ptcb)->tick_node, tick )
#define DELAY_Q_REMOVE(ptcb)            softtimer_remove( &(ptcb)->tick_node )
#define PLIST_PTR_TO_SEMID( ptr )       list_entry( (ptr), semaphore_t, pending_tasks )
#define SEM_MEMBER_PTR_TO_SEMID( ptr )  list_entry( (ptr), mutex_t, sem_member_node )
#define NULL                       ((void*)0)

#undef int_min
#define int_min(a, b) ((a)>(b)?(b):(a))

#if ((MAX_PRIORITY+1)&(32-1))
#define __MAX_GROUPS    ((MAX_PRIORITY+1)/32+1)
#else
#define __MAX_GROUPS    ((MAX_PRIORITY+1)/32)
#endif


struct __priority_q_bitmap_head
{
    pqn_t       *phighest_node;
    unsigned int      bitmap_group;
    uint32_t          bitmap_tasks[__MAX_GROUPS];
    struct list_head  tasks[MAX_PRIORITY+1];
};
typedef struct __priority_q_bitmap_head priority_q_bitmap_head_t;

/*********************************************************************************************************
 **  globle var
 ********************************************************************************************************/
static priority_q_bitmap_head_t  g_readyq;
static struct list_head          g_softtime_head;
volatile unsigned long           g_systick;
tcb_t                           *ptcb_current;
int                              is_int_context;
struct list_head                 g_systerm_tasks_head;

void *memcpy(void*,const void*,int);
static inline void    __put_tcb_to_pendlist( semaphore_t *semid, tcb_t *ptcbToAdd );
static int            __mutex_owner_set( mutex_t *semid, tcb_t *ptcbToAdd );
static inline int     __get_pend_list_priority ( semaphore_t *semid );
static inline int     __get_mutex_hold_list_priority ( tcb_t *ptcb );
static void           __restore_current_task_priority( mutex_t *semid );
static void           task_delay_timeout( softtimer_t *pdn );
static void           priority_q_init( void );
static int            priority_q_put( pqn_t *pNode, int key );
static int            priority_q_remove( pqn_t *pNode );
static void           softtimer_set_func( softtimer_t *pNode, void (*func)(softtimer_t *) );
static void           softtimer_add(softtimer_t *pdn, unsigned int uiTick);
static void           softtimer_remove ( softtimer_t *pdn );
void                  softtimer_announce( void );
static void           __release_one_mutex( mutex_t *semid );
static tcb_t         *highest_tcb_get( void );
extern void           arch_context_switch(void **fromsp, void **tosp);
void                  arch_context_switch_to(void **sp);
static void           schedule_internel( void );
static int            __mutex_raise_owner_priority( mutex_t *semid, int priority );
extern unsigned char *arch_stack_init(void *tentry, void *parameter1, void *parameter2,
                      char *stack_low, char *stack_high, void *texit);
static int            __insert_pend_list_and_trig( semaphore_t *semid, tcb_t *ptcb );
static int            __sem_wakeup_pender( semaphore_t *semid, int err, int count );
#if DEAD_LOCK_DETECT_EN
static int            __mutex_dead_lock_detected( mutex_t * semid );
#endif
#if DEAD_LOCK_SHOW_EN
void                  __mutex_dead_lock_show( mutex_t *mutex );
#endif

static
void priority_q_init( void )
{
    priority_q_bitmap_head_t *pqriHead = &g_readyq;
    int i;

    pqriHead->phighest_node = NULL;

    pqriHead->bitmap_group = 0;

    for (i = 0; i < sizeof(pqriHead->bitmap_tasks)/sizeof(pqriHead->bitmap_tasks[0]); i++) {
        pqriHead->bitmap_tasks[i] = 0;
    }

    for (i = 0; i <= MAX_PRIORITY; i++) {
        INIT_LIST_HEAD( &pqriHead->tasks[i] );
    }
}

static inline
void priority_q_bitmap_set ( priority_q_bitmap_head_t *pqriHead, int priority )
{
    pqriHead->bitmap_tasks[ priority>>5 ] |= 1 << ( 0x1f & priority);
    pqriHead->bitmap_group                |= 1 << (priority>>5);
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
    return list_entry( LIST_FIRST( &pqHead->tasks[ index ] ), pqn_t, node);
}

int priority_q_put( pqn_t *pNode, int key )
{
    register priority_q_bitmap_head_t *pqHead = &g_readyq;

    /* cannot put more than once time, but, if key not the same, we change it.  */
    if ( unlikely(!list_empty(&pNode->node)) ) {
        if ( likely(pNode->key == key) ) {
            return -1;
        } else {
            priority_q_remove( pNode );
        }
    }
    pNode->key = key;
    priority_q_bitmap_set( pqHead, key );
    list_add_tail( &pNode->node, &pqHead->tasks[key] );

    /*
     *  set high node
     */
    if ( unlikely(NULL == pqHead->phighest_node) || (key < pqHead->phighest_node->key) ) {
        pqHead->phighest_node = pNode;
    }
    return 0;
}

int priority_q_remove( pqn_t *pNode )
{
    register priority_q_bitmap_head_t *pqHead = &g_readyq;
    int key;
    
    key = pNode->key;
    list_del_init( &pNode->node );
    if ( list_empty( &pqHead->tasks[key] ) ) {
        priority_q_bitmap_clear( pqHead, key );
    }
    if ( pqHead->phighest_node == pNode  ) {
        pqHead->phighest_node = priority_q_highest_get( pqHead );
    }
    return 0;
}

void task_delay( int tick )
{
    int old;
    int last;

    old = arch_interrupt_disable();
    READY_Q_REMOVE( ptcb_current );
    last = ptcb_current->err;
    ptcb_current->status = TASK_DELAY;
    softtimer_add( &ptcb_current->tick_node, tick );
    schedule_internel();
    ptcb_current->err = last;
    ptcb_current->status = TASK_READY;
    arch_interrupt_enable(old);
}

static
void task_delay_timeout( softtimer_t *pNode )
{
    tcb_t *p;
    
    p = TICK_NODE_TO_PTCB( pNode );
    if ( list_empty( &p->prio_node.node ) ) {
    	p->err = ETIME;
        READY_Q_PUT( p, p->current_priority );
    }
}

static
tcb_t *highest_tcb_get( void )
{
    if ( ptcb_current->status == TASK_READY ) {
        READY_Q_REMOVE( ptcb_current );
        READY_Q_PUT( ptcb_current, ptcb_current->current_priority );
    }
    return PRIO_NODE_TO_PTCB( g_readyq.phighest_node );
}

void os_startup( void )
{
    tcb_t *ptcb = PRIO_NODE_TO_PTCB( g_readyq.phighest_node );
    arch_context_switch_to(&ptcb->sp);
}

void softtimer_add(softtimer_t *pdn, unsigned int uiTick)
{
    struct list_head *p;
    softtimer_t      *pDelayNode=0;
    
    list_for_each( p, &g_softtime_head ) {
        pDelayNode = list_entry(p, softtimer_t, node);
        if (uiTick > pDelayNode->uiTick ) {
            uiTick -= pDelayNode->uiTick;
        } else {
            break;
        }
    }
    pdn->uiTick = uiTick;

    list_add_tail( &pdn->node, p);
    if (p != &g_softtime_head ) {
        pDelayNode->uiTick -= uiTick;
    }
}

void softtimer_remove ( softtimer_t *pdn )
{
    softtimer_t *pNextNode;

    if ( list_empty(&pdn->node) ) {
        return ;
    }

    if ( LIST_FIRST( &pdn->node ) != &g_softtime_head ) {
        pNextNode = list_entry( LIST_FIRST( &pdn->node ), softtimer_t, node);
        pNextNode->uiTick += pdn->uiTick;
    }
    list_del_init( &pdn->node );
}

void softtimer_set_func( softtimer_t *pNode, void (*func)(softtimer_t *) )
{
    pNode->timeout_func = func;
}

/**
 *  \brief soft timer announce.
 *
 *  systerm tick is provided by calling this function.
 *
 *  \sa ENTER_INT_CONTEXT(), EXIT_INT_CONTEXT().
 */
void softtimer_announce( void )
{
    int old = arch_interrupt_disable();
    
    g_systick++;
    
    if ( !list_empty( &g_softtime_head ) ) {
        struct list_head *p;
        struct list_head *pNext;
        softtimer_t *pNode;
        
        p = LIST_FIRST( &g_softtime_head );
        pNode = list_entry(p, softtimer_t, node);

        /*
         * in case of 'task_delay(0)'
         */
        if ( likely(pNode->uiTick) ) {
            pNode->uiTick--;
        }
        
        for (; !list_empty(&g_softtime_head); ) {
            pNext = LIST_FIRST(p);
            pNode = list_entry(p, softtimer_t, node);
            if ( pNode->uiTick == 0) {
                list_del_init( &pNode->node );
                if ( pNode->timeout_func ) {
                    (*pNode->timeout_func)( pNode );
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
void __sem_init_common( semaphore_t *semid )
{
    semid->u.count               = 0;
    INIT_LIST_HEAD( &semid->pending_tasks );
}

/**
 * \addtogroup SEMAPHORE_API    semaphore API
 * @{
 */

/**
 *  \brief Initialize a counter semaphore.
 *
 *  \param[in]  semid       pointer
 *  \param[in]  initcount   Initializer: 0 or 1.
 *  \return     0           always successfully.
 *  \attention  parameter is not checked. You should check it by yourself.
 */
int semc_init( semaphore_t *semid, int InitCount )
{
    __sem_init_common( semid );
    semid->u.count = InitCount;
    semid->type  = SEM_TYPE_COUNTER;
    return 0;
}

/**
 *  \brief Initialize a binary semaphore.
 *
 *  \param[in]  semid       pointer
 *  \param[in]  initcount   Initializer: 0 or 1.
 *  \return     0           always successfully.
 *  \attention  parameter is not checked. You should check it by yourself.
 */
int semb_init( semaphore_t *semid, int initcount )
{
    __sem_init_common( semid );
    semid->u.count = !!initcount;
    semid->type  = SEM_TYPE_BINARY;
    return 0;
}

/**
 *  \brief reset a semaphore binary.
 *
 *  \sa semb_init(), semb_take(), semb_give(), semb_terminate()
 */
int semb_clear( semaphore_t *semid )
{
    int old;
    
    old = arch_interrupt_disable();
    semid->u.count = 0;
    arch_interrupt_enable(old );
    return 0;
}

/**
 *  \brief reset a semaphore counter.
 *
 *  \sa semc_init(), semc_take(), semc_give(), semc_terminate()
 */
int semc_clear( semaphore_t *semid )
{
    return semb_clear(semid);
}

/**
 *  \brief initialize a mutex.
 *  
 *  \param[in]  semid       pointer
 *  \return     0           always successfully.
 *  \attention  parameter is not checked. You should check it yourself.
 *  \sa mutex_terminate()
 */
int mutex_init( mutex_t *semid )
{
    __sem_init_common( (semaphore_t*)semid );
    INIT_LIST_HEAD( &semid->sem_member_node );
    semid->mutex_recurse_count  = 0;
    semid->s.type                 = SEM_TYPE_MUTEX;
    return 0;
}

static
int __sem_terminate( semaphore_t *semid )
{
    int               old;
    int               happen = 0;

    old = arch_interrupt_disable();
    happen = __sem_wakeup_pender(semid, ENXIO, -1 );
    if ( happen ) {
        schedule_internel();
    }
    arch_interrupt_enable(old);

    return happen;
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
int semb_terminate( semaphore_t *semid )
{
    return __sem_terminate(semid);
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
int semc_terminate( semaphore_t *semid )
{
    return __sem_terminate(semid);
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
int mutex_terminate( mutex_t *semid )
{
    int old;

    old = arch_interrupt_disable();
    __mutex_owner_set( semid, NULL );
    __sem_terminate( (semaphore_t*)semid );
    arch_interrupt_enable( old );
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
int semb_take( semaphore_t *semid, unsigned int tick )
{
    int old;
    int TaskStatus = 0;

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
    if ( tick != WAIT_FOREVER ) {
        TaskStatus = TASK_DELAY;
        softtimer_add( &ptcb_current->tick_node, tick );
    }
    ptcb_current->psem_list = &semid->pending_tasks;
    ptcb_current->status    = TASK_PENDING | TaskStatus;
    ptcb_current->err       = 0;
    __put_tcb_to_pendlist( semid, ptcb_current );
    do {
        READY_Q_REMOVE( ptcb_current );
        schedule_internel();
        if ( ( ptcb_current->err==0|| ptcb_current->err==ETIME ) && semid->u.count ) {
            semid->u.count = 0;
            break;
        }
    } while ( !ptcb_current->err );
	list_del_init( &ptcb_current->sem_node );
	ptcb_current->status    = TASK_READY;
	ptcb_current->psem_list = NULL;
    softtimer_remove( &ptcb_current->tick_node );
    arch_interrupt_enable( old );
    return -ptcb_current->err;
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
int semc_take( semaphore_t *semid, unsigned int tick )
{
    int old;
    int TaskStatus = 0;

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
    if ( tick != WAIT_FOREVER ) {
        TaskStatus = TASK_DELAY;
        softtimer_add( &ptcb_current->tick_node, tick );
    }
    ptcb_current->psem_list      = &semid->pending_tasks;
    ptcb_current->err          = 0;
    ptcb_current->status = TASK_PENDING | TaskStatus;
    __put_tcb_to_pendlist( semid, ptcb_current );
    do {
        READY_Q_REMOVE( ptcb_current );
        schedule_internel();
        if ((ptcb_current->err==0||ptcb_current->err==ETIME) && semid->u.count ) {
            semid->u.count--;
            break;
        }
    } while ( !ptcb_current->err );
	ptcb_current->status    = TASK_READY;
	list_del_init( &ptcb_current->sem_node );
	ptcb_current->psem_list = NULL;
    softtimer_remove( &ptcb_current->tick_node );
    arch_interrupt_enable(old );
    return -ptcb_current->err;
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
int mutex_lock( mutex_t *semid, unsigned int tick )
{
    int old;
    int TaskStatus = 0;

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
        __mutex_owner_set( semid, ptcb_current );
        semid->mutex_recurse_count++;
        arch_interrupt_enable(old );
        return 0;
    } else if ( semid->s.u.owner == ptcb_current ) {
        semid->mutex_recurse_count++;
        arch_interrupt_enable(old );
        return 0;
    }
#if DEAD_LOCK_DETECT_EN
    else if ( __mutex_dead_lock_detected( semid ) ) {
#ifdef DEAD_LOCK_HOOK
    	DEAD_LOCK_HOOK(semid, semid->s.u.owner );
#endif
#if DEAD_LOCK_SHOW_EN
    	__mutex_dead_lock_show( semid );
#endif
    	return -EDEADLK;/* Deadlock condition */
    }
#endif
    if ( tick == 0 ) {
        arch_interrupt_enable(old );
        return -EAGAIN;
    }
    if ( tick != WAIT_FOREVER ) {
        TaskStatus = TASK_DELAY;
        softtimer_add( &ptcb_current->tick_node, tick );
    }
    ptcb_current->status    = TASK_PENDING | TaskStatus;
    ptcb_current->psem_list = &semid->s.pending_tasks;
    ptcb_current->err       = 0;
    /*
     *  put tcb into pend list and inherit priority.
     */
    if ( __insert_pend_list_and_trig( (semaphore_t*)semid, ptcb_current ) ) {
        __mutex_raise_owner_priority( semid, ptcb_current->current_priority );
    }
    do {
        READY_Q_REMOVE( ptcb_current );
        schedule_internel();
        if (( ptcb_current->err==0|| ptcb_current->err==ETIME ) &&  semid->s.u.owner == NULL ) {
            __mutex_owner_set( semid, ptcb_current );
            semid->mutex_recurse_count = 1;
            break;
        }
    } while ( !ptcb_current->err );
	ptcb_current->status = TASK_READY;
	list_del_init( &ptcb_current->sem_node );
	ptcb_current->psem_list = NULL;
    softtimer_remove( &ptcb_current->tick_node );
    arch_interrupt_enable(old );
    return -ptcb_current->err;
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
int mutex_unlock( mutex_t *semid )
{
    int old;
    
#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(semid->s.type != SEM_TYPE_MUTEX) ) {
        return -EINVAL;
    }
    if ( unlikely(IS_INT_CONTEXT()) ) {
        return -EPERM;
    }
#endif
    old = arch_interrupt_disable();
    if ( semid->s.u.owner != ptcb_current ) {
        arch_interrupt_enable(old );
        return -EPERM;
    }
    if ( --semid->mutex_recurse_count ) {
        arch_interrupt_enable(old );
        return 0;
    }
    
    __release_one_mutex( semid );
    __restore_current_task_priority( semid );
    schedule_internel();
    
    arch_interrupt_enable(old );
    return 0;
}

#if DEAD_LOCK_DETECT_EN
static int __mutex_dead_lock_detected( mutex_t *semid )
{
	tcb_t *powner;
	mutex_t *s = semid;
	powner = s->s.u.owner;
again:
	if ( powner->status & TASK_PENDING ) {
		s = (mutex_t *)PLIST_PTR_TO_SEMID( powner->psem_list );
		if ( s->s.type == SEM_TYPE_MUTEX ) {
			powner = s->s.u.owner;
			if ( powner == ptcb_current  )
				return 1;
			goto again;
		}
	}
	return 0;
}
#if DEAD_LOCK_SHOW_EN
void __mutex_dead_lock_show( mutex_t *mutex )
{
	tcb_t *powner;
	mutex_t *s = mutex;
	powner = s->s.u.owner;

	kprintf("Dead lock path:\n task %s pending on 0x%08X", ptcb_current->name, mutex);
again:
	kprintf(", taken by %s", powner->name );
	if ( powner->status & TASK_PENDING ) {
		s = (mutex_t *)PLIST_PTR_TO_SEMID( powner->psem_list );
		kprintf(", and pending on 0x%08X ", s );
		if ( s->s.type == SEM_TYPE_MUTEX ) {
			powner = s->s.u.owner;
			if ( powner == ptcb_current ) {
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

/**
 *  \brief release a binary semaphore.
 *  \param[in] semid    pointer
 *  \return     0       successfully.
 *  \return     -EPERM  permission denied.
 *  \return     -EINVAL Invalid argument
 *  \note               can be used in interrupt service.
 */
int semb_give( semaphore_t *semid )
{
    int old;

#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(semid->type != SEM_TYPE_BINARY) ) {
        return -EPERM;
    }
#endif

    old = arch_interrupt_disable();
    
    semid->u.count = 1;
    if ( __sem_wakeup_pender( semid, 0, 1 ) && !IS_INT_CONTEXT() ) {
        schedule_internel();
    }
    arch_interrupt_enable(old );
    return 0;
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
int semc_give( semaphore_t *semid )
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
    if ( __sem_wakeup_pender( semid, 0, 1 ) && !IS_INT_CONTEXT() ) {
        schedule_internel();
    }
    arch_interrupt_enable(old );
    return 0;
}
/** @} */

static
void __release_one_mutex( mutex_t *semid )
{
    __sem_wakeup_pender( (semaphore_t*)semid, 0, 1 );
    __mutex_owner_set( semid, NULL );
}

/*
 * ���semid�ĵȴ���е����ȼ���
 * ���е����ȼ�����Ϊ�ö�������ߵ����ȼ�����ֵ��
 */
static inline
int __get_pend_list_priority ( semaphore_t *semid )
{
    tcb_t *ptcb;
    
    if ( !list_empty(&semid->pending_tasks) ) {
        ptcb = PEND_NODE_TO_PTCB( LIST_FIRST(&semid->pending_tasks) );
        return ptcb->current_priority;
    }
    return MAX_PRIORITY+1;
}

/*
 * ���semid�ĵȴ���е����ȼ���
 * ���е����ȼ�����Ϊ�ö�������ߵ����ȼ�����ֵ��
 */
static inline
int __get_mutex_hold_list_priority ( tcb_t *ptcb )
{
    mutex_t *semid;
    
    if ( !list_empty(&ptcb->mutex_holded_head) ) {
        semid = SEM_MEMBER_PTR_TO_SEMID( LIST_FIRST(&ptcb->mutex_holded_head) );
        return __get_pend_list_priority((semaphore_t*)semid);
    }
    return MAX_PRIORITY+1;
}

/*
 * ��������ƿ��ŵ������С������ȼ��ߵ���˳���š�
 */
static inline
void __put_tcb_to_pendlist( semaphore_t *semid, tcb_t *ptcbToAdd )
{
    tcb_t            *ptcb;
    struct list_head *p;

    list_for_each( p, &semid->pending_tasks) {
        ptcb = PEND_NODE_TO_PTCB( p );
        if ( ptcb->current_priority > ptcbToAdd->current_priority ) {
            break;
        }
    }
    
    list_add_tail( &ptcbToAdd->sem_node, p );
}

/*
 * ���û������������ߡ�
 */
static
int  __mutex_owner_set( mutex_t *semid, tcb_t *ptcbToAdd )
{
    struct list_head *p;
    mutex_t          *psem;
    int               pri;

    if ( NULL == ptcbToAdd ) {
        semid->s.u.owner = NULL;
        list_del_init( &semid->sem_member_node );
        return 0;
    }

    semid->s.u.owner = ptcbToAdd;

    pri = __get_pend_list_priority((semaphore_t*)semid);
    list_for_each( p, &ptcbToAdd->mutex_holded_head) {
        psem = SEM_MEMBER_PTR_TO_SEMID( p );
        if ( __get_pend_list_priority( (semaphore_t*)psem ) > pri )  {
            break;
        }
    }

    list_add_tail( &semid->sem_member_node, p );
    return 0;
}

/*
 * ����semid�ȴ���������ȼ���ߵ����񡣻��Ѳ���Ϊerr.
 * ��errΪ0ʱ�����Ƴ�ʱ����
 */
static
int __sem_wakeup_pender( semaphore_t *semid, int err, int count )
{
    register int               n;
    register struct list_head *p;
    register struct list_head *save;
    register tcb_t            *ptcbwakeup;

    n = 0;
    list_for_each_safe( p, save, &semid->pending_tasks ) {
        ptcbwakeup = PEND_NODE_TO_PTCB( p );
        if ( err ) {
            /* remove timer only error. Otherwise */
            softtimer_remove( &ptcbwakeup->tick_node );
            list_del_init( &ptcbwakeup->sem_node );
        }
        READY_Q_PUT( ptcbwakeup, ptcbwakeup->current_priority );
        ptcbwakeup->err = err;
        if ( ++n == count ) {
            break;
        }
    }
    return n;
}


/**
 *  ����ptcbOwner->current_priority�ı仯��semid��pending_tasks������Ҫ����.
 *  ���ص�����Ƿ�����������е����ȼ�(��ߵ��Ǹ���������ȼ�)��
 */
static
int __resort_hold_mutex_list_and_trig( mutex_t *semid, tcb_t *ptcbOwner )
{
    int pri;
    
    __mutex_owner_set(semid, NULL);
    pri = __get_mutex_hold_list_priority(ptcbOwner);
    __mutex_owner_set(semid, ptcbOwner);
    return pri > __get_mutex_hold_list_priority(ptcbOwner);
}

/**
 *  ����ptcb->current_priority�ı仯��semid��pending_tasks������Ҫ����.
 *  ���ص�����Ƿ�����������е����ȼ�(��ߵ��Ǹ���������ȼ�)��
 */
static
int __resort_pend_list_and_trig( semaphore_t *semid, tcb_t *ptcb )
{
    int pri;
    
    list_del_init( &ptcb->sem_node );
    pri = __get_pend_list_priority(semid);
    __put_tcb_to_pendlist( semid, ptcb );
    return pri > __get_pend_list_priority(semid);
}

/**
 * ��������ƿ���뵽semid�ĵȴ���С��������Ƿ�ı���е����ȼ���
 */
static
int __insert_pend_list_and_trig( semaphore_t *semid, tcb_t *ptcb )
{
    int pri;
    
    pri = __get_pend_list_priority(semid);
    __put_tcb_to_pendlist( semid, ptcb );
    return pri > __get_pend_list_priority(semid);
}


/**
 *  \addtogroup TASK_API    task API
 *  @{
 *  
 *  @brief set task priority 
 *  @fn task_priority_set
 *  @param[in]  ptcb            task control block pointer. If NULL, current task's
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
int task_priority_set( tcb_t *ptcb, unsigned int priority )
{
    int old;
    int ret = 0;
    int need = 0;/* need to call scheduler */

    if ( ptcb == NULL ) {
        ptcb = ptcb_current;
    }
    if ( priority > MAX_PRIORITY ) {
        return -EINVAL;
    }
    old = arch_interrupt_disable();

    if ( TASK_PREPARED == ptcb->status ||
    	 TASK_DEAD == ptcb->status  ) {
    	ret = -EPERM;
    	goto done;
    }
    if ( ptcb->priority == priority ) {
        goto done;
    }
    
    ptcb->priority = priority;
    if ( priority < ptcb->current_priority ) {     /* priority goes up */
        ptcb->current_priority = priority;
    } else if ( __get_mutex_hold_list_priority( ptcb ) >= priority ) {
        ptcb->current_priority = priority;/* priority can go down at the moment. */
    }

    if ( ptcb->status & TASK_PENDING ) {
        semaphore_t *semid;
        int          trig;
        semid   = PLIST_PTR_TO_SEMID( ptcb->psem_list );
        trig = __resort_pend_list_and_trig( (semaphore_t*)semid, ptcb );
        if ( trig && semid->type == SEM_TYPE_MUTEX ) {
            need = __mutex_raise_owner_priority( (mutex_t*)semid, priority );
        }
    }

    if ( (ptcb->status == TASK_READY) &&
         (ptcb->current_priority == priority) ) {
        READY_Q_REMOVE( ptcb );
        READY_Q_PUT( ptcb, ptcb->current_priority );
        need = 1;
    }
    if ( need && !IS_INT_CONTEXT()) {
        schedule_internel();
    }
    
done:
    arch_interrupt_enable( old );
    return ret;
}

/** @} */

static
int __mutex_raise_owner_priority( mutex_t *semid, int priority )
{
    int ret = 0;
    tcb_t *powner;
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
                semid   = (mutex_t*)PLIST_PTR_TO_SEMID( powner->psem_list );
                if ( __resort_pend_list_and_trig( (semaphore_t*)semid, powner ) &&
                     (semid->s.type == SEM_TYPE_MUTEX) && semid->s.u.owner ) {
                    powner = semid->s.u.owner;
                    goto again;
                }
            }
        }
    }
    return ret;
}

static
void __restore_current_task_priority ( mutex_t *semid )
{
    int priority;

    /*
     *  find the highest priority needed to setup, 
     *  which is from the mutex of current task holded.
     *  it will be always the first one of MutexHeadList.
     */
    priority = __get_mutex_hold_list_priority(ptcb_current);
    if ( priority > ptcb_current->priority ) {
        priority = ptcb_current->priority;
    }
    
    if ( unlikely(priority != ptcb_current->current_priority )) {
        READY_Q_REMOVE( ptcb_current );
        READY_Q_PUT( ptcb_current, priority );
        ptcb_current->current_priority = priority;
    }
}

static
void task_exit( void )
{
    tcb_t *ptcb;
    
    arch_interrupt_disable();
    ptcb_current->status = TASK_DEAD;
    READY_Q_REMOVE( ptcb_current );
    list_del_init( &ptcb_current->task_list_node );
    
    ptcb = PRIO_NODE_TO_PTCB( g_readyq.phighest_node );
    arch_context_switch_to(&ptcb->sp);
}

/**
 *  \brief stop a task.
 *
 *  \param[in]  ptcb    task control block pointer.
 *                      If NULL, it will equal to ptcb_current.
 *
 *  \return     0       successfully.
 *  \return     -EPERM  Permission denied:
 *                      It is protected by calling task_safe().
 */
int task_terminate( tcb_t *ptcb )
{
    int old;
    int ret = 0;
    struct list_head *save, *p;

    if ( ptcb == NULL ) {
        ptcb = ptcb_current;
    }
    
    old = arch_interrupt_disable();
    if ( ptcb->safe_count ) {
        ret = -EPERM;
        goto done;
    }
    
    /*
     *  remove delay node
     */
    if ( !list_empty( &(ptcb->tick_node.node) )) {
        softtimer_remove( &ptcb->tick_node );
    }

    /*
     *  release all mutex. 
     */
    list_for_each_safe(p, save, &ptcb->mutex_holded_head){
        mutex_t *psemid;
        psemid = SEM_MEMBER_PTR_TO_SEMID( p );
        __release_one_mutex( psemid );
    }
    
    READY_Q_REMOVE( ptcb );
    list_del_init( &ptcb->task_list_node );
    ptcb->status = TASK_DEAD;
    schedule_internel();
    
done:
    arch_interrupt_enable(old );
    return ret;
}

int task_startup( tcb_t *ptcb )
{
    int old;
    int ret = 0;
    
    old = arch_interrupt_disable();
    if ( ptcb->status != TASK_PREPARED ) {
        arch_interrupt_enable(old );
        return -EPERM;
    }

    list_add_tail( &ptcb->task_list_node, &g_systerm_tasks_head );
    READY_Q_PUT( ptcb, ptcb->current_priority );
    ptcb->status = TASK_READY;
    /*
     *  do not call scheduler while kernel is not running ( at startup point ).
     */
    if ( NULL != ptcb_current ) {
        schedule_internel();
        ret = 0;
    }
    
    arch_interrupt_enable(old );
    return ret;
}

void task_init(tcb_t      *ptcb, 
               const char *name,
               int         priority, /* priority of new task */
               int         option, /* task option word */
               char *      stack_low,
               char *      stack_high,
               void       *pfunc, /* entry point of new task */
               void       *arg1, /* 1st of 10 req'd args to pass to entryPt */
               void       *arg2)
{
    ptcb->priority         = priority;
    ptcb->current_priority = priority;
    ptcb->status           = TASK_PREPARED;
    ptcb->psem_list        = (void*)0;
    ptcb->option           = option;
    ptcb->stack_low        = stack_low;
    ptcb->stack_high       = stack_high;
    ptcb->safe_count       = 0;
    ptcb->name = name;

    ptcb->sp = arch_stack_init( pfunc, arg1, arg2, stack_low, stack_high, task_exit );
    INIT_LIST_HEAD( &ptcb->prio_node.node );
    INIT_LIST_HEAD( &ptcb->tick_node.node );
    INIT_LIST_HEAD( &ptcb->sem_node );
    INIT_LIST_HEAD( &ptcb->mutex_holded_head );
    INIT_LIST_HEAD( &ptcb->task_list_node );
    softtimer_set_func( &ptcb->tick_node, task_delay_timeout );
}

int task_safe( void )
{
    return ++ptcb_current->safe_count;
}
int task_unsafe( void )
{
    return --ptcb_current->safe_count;
}

static
void schedule_internel( void )
{
    tcb_t *p;

    p = highest_tcb_get();
    if ( p != ptcb_current) {
        arch_context_switch( &ptcb_current->sp, &p->sp);        
    }
}

/*
 *  this function must keep very simple.
 */
void set_ptcb_current( void **pp )
{
    ptcb_current = container_of(pp, tcb_t, sp);
}

void schedule( void )
{
    int    old;
    tcb_t *p;

    old = arch_interrupt_disable();
    if ( IS_INT_CONTEXT() ) {
        goto done;
    }
    
    p = highest_tcb_get();
    if ( p != ptcb_current ){
        arch_context_switch( &ptcb_current->sp, &p->sp);        
    }
  done:
    arch_interrupt_enable(old);
}

static
void task_idle( void *arg )
{
    task_safe();
    while (1) {
#ifdef IDLE_TASK_HOOK
        IDLE_TASK_HOOK;
#endif
        schedule();
    }
}

void kernel_init( void )
{
    TASK_INFO_DECL(static, info1, 256 );
    
    INIT_LIST_HEAD( &g_systerm_tasks_head );
    INIT_LIST_HEAD(&g_softtime_head);
    priority_q_init();
    TASK_INIT( "idle", info1, MAX_PRIORITY, task_idle, 0,0 );
    TASK_STARTUP(info1);
}

unsigned int tick_get( void )
{
    return g_systick;
}

/**
 *  \brief Initialize a msgq.
 *
 *  \param[in]  pmsgq       pointer
 *  \param[in]  buff        buffer pointer.
 *  \param[in]  buffer_size buffer size.
 *  \param[in]  unit_size   element size.
 *  \return     0           always successfully.
 *  \return     -EINVAL     Invalid argument.
 *  \attention  parameter is not checked. You should check it by yourself.
 */
int msgq_init( msgq_t *pmsgq, void *buff, int buffer_size, int unit_size )
{
    int     count;

    count = buffer_size / unit_size;
    
    if ( buffer_size == 0 || count == 0 ) {
        return -EINVAL;
    }

    pmsgq->buff_size = buffer_size;
    pmsgq->rd        = 0;
    pmsgq->wr        = 0;
    pmsgq->unit_size = unit_size;
    pmsgq->count     = count;
    pmsgq->buff      = buff;

    semc_init( &pmsgq->sem_rd,  0 );
    semc_init( &pmsgq->sem_wr,  count );

    return 0;
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
int msgq_terminate( msgq_t *pmsgq )
{
    int old;

    old = arch_interrupt_disable();
    semb_terminate( &pmsgq->sem_rd );
    semb_terminate( &pmsgq->sem_wr );
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
 *  @retval -1       error, please check errno. if errno == ETIME, it means Timer expired,
 *                   if errno == ENOMEM, it mean buffer_size if not enough.
 *  @retval 0        receive successfully.
 */
int msgq_receive( msgq_t *pmsgq, void *buff, int buff_size, int tick )
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
    pmsgq->rd = (pmsgq->rd + 1) % pmsgq->count;

    arch_interrupt_enable(old);

    if ( buff ) {
        memcpy( buff, pmsgq->buff + pmsgq->unit_size*rd, pmsgq->unit_size );
    }

    semc_give( &pmsgq->sem_wr );
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
int msgq_send( msgq_t *pmsgq, const void *buff, int size, int tick )
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
    
    arch_interrupt_enable(old);

    if ( buff ) {
        memcpy( pmsgq->buff + pmsgq->unit_size*wr, buff, int_min(pmsgq->unit_size, size) );
    }

    semc_give( &pmsgq->sem_rd );
    return 0;
}

int msgq_clear( msgq_t *pmsgq )
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

    n = __sem_wakeup_pender(&pmsgq->sem_wr, 0, pmsgq->sem_wr.u.count ) ;
    
    if ( n && !IS_INT_CONTEXT() ) {
        schedule_internel();
    }
    arch_interrupt_enable(old);
    return 0;
}

