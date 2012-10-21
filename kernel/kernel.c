/* Last modified Time-stamp: <2012-10-21 09:30:15 Sunday by lyzh>
 * @(#)kernel.c
 */
#include "list.h"
#include "os.h"
#include "err.h"

#define PRIO_NODE_TO_PTCB(pNode)        list_entry(pNode, tcb_t, prio_node)
#define TICK_NODE_TO_PTCB(pNode)        list_entry(pNode, tcb_t, tick_node)
#define PEND_NODE_TO_PTCB(pNode)        list_entry(pNode, tcb_t, sem_node)
#define READY_Q_REMOVE( ptcb )          priority_q_remove( &g_readyq, &(ptcb)->prio_node )
#define READY_Q_PUT( ptcb, key)         priority_q_put(&g_readyq, &(ptcb)->prio_node, key)
#define DELAY_Q_PUT(ptcb, tick)         softtimer_add( &(ptcb)->tick_node, tick )
#define DELAY_Q_REMOVE(ptcb)            softtimer_remove( &(ptcb)->tick_node )
#define PLIST_PTR_TO_SEMID( ptr )       list_entry( (ptr), semaphore_t, pending_tasks )
#define SEM_MEMBER_PTR_TO_SEMID( ptr )  list_entry( (ptr), semaphore_t, sem_member_node )
#define NULL                       ((void*)0)
#define delay_node_init( p, tick )              \
    do {                                        \
        softtimer_t *pdn = (p);                 \
        INIT_LIST_HEAD(&(pdn)->node);           \
        (pdn)->uiTick    = (tick);              \
    } while (0)

#define inline
#undef int_min
#define int_min(a, b) ((a)>(b)?(b):(a))


struct __priority_q_bitmap_head
{
    pqn_t       *phighest_node; 
    unsigned int      bitmap_group;
    uint32_t          bitmap_tasks[MAX_PRIORITY/32+1];
    struct list_head  tasks[MAX_PRIORITY];
};
typedef struct __priority_q_bitmap_head priority_q_bitmap_head_t;

/*********************************************************************************************************
 **  globle var
 ********************************************************************************************************/
static priority_q_bitmap_head_t  g_readyq;
static struct list_head          GlistDelayHead;
volatile unsigned long           g_systick;
tcb_t                           *ptcb_current;
int                              is_int_context;
struct list_head                 GlistAllTaskListHead;

static inline void                          __put_tcb_to_pendlist( semaphore_t *semid, tcb_t *ptcbToAdd );
static int                                  __mutex_owner_set( semaphore_t *semid, tcb_t *ptcbToAdd );
static inline int                           __get_pend_list_priority ( semaphore_t *semid );
static inline int                           __get_mutex_hold_list_priority ( tcb_t *ptcb );
static inline struct list_head             *__sem_pend_list_get_and_remove_first_node( semaphore_t *semid );
static void                                 __mutex_priority_inheritance( semaphore_t *semid );
static void                                 __restore_current_task_priority( semaphore_t *semid );
static void                                 task_delay_timeout( softtimer_t *pdn );
static void                                 priority_q_init( priority_q_bitmap_head_t *pqriHead );
static int                                  priority_q_put( priority_q_bitmap_head_t *pqHead, pqn_t *pNode, int key );
static int                                  priority_q_remove( priority_q_bitmap_head_t *pqHead, pqn_t *pNode );
static void                                 softtimer_set_func( softtimer_t *pNode, void (*func)(softtimer_t *) );
static void                                 softtimer_add(softtimer_t *pdn, unsigned int uiTick);
static void                                 softtimer_remove ( softtimer_t *pdn );
void                                        softtimer_anounce( void );
void                                        context_switch_start( void );
void                                        __release_holded_mutex( tcb_t *ptcb );
static void                                 __release_one_mutex( semaphore_t *semid );
static tcb_t                               *highest_tcb_get( void );
extern void                                 arch_context_switch(void **fromsp, void **tosp);
extern void                                 arch_context_switch_interrupt(void **fromsp, void **tosp);
void                                        arch_context_switch_to(void **sp);
static void                                 schedule_internel( void );
extern unsigned char *arch_stack_init(void *tentry, void *parameter1, void *parameter2,
                                      char        *stack_low, char *stack_high, void *texit);

static void priority_q_init( priority_q_bitmap_head_t *pqriHead )
{
    int i;

    pqriHead->phighest_node = NULL;

    pqriHead->bitmap_group = 0;

    for (i = 0; i < sizeof(pqriHead->bitmap_tasks)/sizeof(pqriHead->bitmap_tasks[0]); i++) {
        pqriHead->bitmap_tasks[i] = 0;
    }

    for (i = 0; i < MAX_PRIORITY; i++) {
        INIT_LIST_HEAD( &pqriHead->tasks[i] );
    }
}

static inline void priority_q_bitmap_set ( priority_q_bitmap_head_t *pqriHead, int priority )
{
    pqriHead->bitmap_tasks[ priority>>5 ] |= 1 << ( 0x1f & priority);
    pqriHead->bitmap_group                |= 1 << (priority>>5);
}

static inline void priority_q_bitmap_clear( priority_q_bitmap_head_t *pqriHead, int priority )
{
    int group                         = priority>>5;
    pqriHead->bitmap_tasks[ group  ] &= ~(1 << ( 0x1f & priority));
    
    if ( unlikely(0 == pqriHead->bitmap_tasks[ group  ]) ) {
        pqriHead->bitmap_group &= ~(1 << group);
    }
}

static pqn_t *priority_q_highest_get( priority_q_bitmap_head_t *pqHead )
{
    int      index;
    int      i;
    
    if ( unlikely(!pqHead->bitmap_group) ) {
        return NULL;
    }
    
    i = ffs( pqHead->bitmap_group ) - 1;
    index = ffs( pqHead->bitmap_tasks[i] ) + (i << 5) - 1;
    return list_entry( LIST_FIRST( &pqHead->tasks[ index ] ), pqn_t, node);
}

int priority_q_put( priority_q_bitmap_head_t *pqHead, pqn_t *pNode, int key )
{
    ASSERT( key < 256 && key >= 0 );
    ASSERT( NULL != pqHead && pNode != NULL );
	
    pNode->key = key;
    priority_q_bitmap_set( pqHead, key );
    list_add_tail( &pNode->node, &pqHead->tasks[key] );

    /*
     *  set high node
     */
    if ( (NULL == pqHead->phighest_node) || (key < pqHead->phighest_node->key) ) {
        pqHead->phighest_node = pNode;
    }
    return 0;
}

int priority_q_remove( priority_q_bitmap_head_t *pqHead, pqn_t *pNode )
{
    int key;
    
    ASSERT( NULL != pqHead && pNode != NULL );

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

    old = arch_interrupt_disable();
    READY_Q_REMOVE( ptcb_current );
    ptcb_current->status |= TASK_DELAYD;
    softtimer_add( &ptcb_current->tick_node, tick );
    schedule_internel();
    arch_interrupt_enable(old);
}

static void task_delay_timeout( softtimer_t *pNode )
{
    tcb_t *p;
    
    p = TICK_NODE_TO_PTCB( pNode );
    p->err = ETIME;
    list_del_init( &p->sem_node );
    
    p->status = TASK_READY;

    if ( list_empty( &p->prio_node.node ) ) {
        READY_Q_PUT( p, p->current_priority );
    }
}
static tcb_t *highest_tcb_get( void )
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
    
    list_for_each( p, &GlistDelayHead ) {
        pDelayNode = list_entry(p, softtimer_t, node);
        if (uiTick > pDelayNode->uiTick ) {
            uiTick -= pDelayNode->uiTick;
        } else {
            break;
        }
    }
    pdn->uiTick = uiTick;

    list_add_tail( &pdn->node, p);
    if (p != &GlistDelayHead ) {
        pDelayNode->uiTick -= uiTick;
    }
}

void softtimer_remove ( softtimer_t *pdn )
{
    softtimer_t *pNextNode;

    if ( list_empty(&pdn->node) ) {
        return ;
    }

    if ( LIST_FIRST( &pdn->node ) != &GlistDelayHead ) {
        pNextNode = list_entry( LIST_FIRST( &pdn->node ), softtimer_t, node);
        pNextNode->uiTick += pdn->uiTick;
    }
    list_del_init( &pdn->node );
}

void softtimer_set_func( softtimer_t *pNode, void (*func)(softtimer_t *) )
{
    pNode->timeout_func = func;
}

void softtimer_anounce( void )
{
    int old = arch_interrupt_disable();
    
    g_systick++;
    
    if ( !list_empty( &GlistDelayHead ) ) {
        struct list_head *p;
        struct list_head *pNext;
        softtimer_t *pNode;
        
        p = LIST_FIRST( &GlistDelayHead );
        pNode = list_entry(p, softtimer_t, node);

        /*
         * in case of 'task_delay(0)'
         */
        if ( likely(pNode->uiTick) ) {
            pNode->uiTick--;
        }
        
        for (; !list_empty(&GlistDelayHead); ) {
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

static void __sem_init_common( semaphore_t *semid )
{
    semid->u.count               = 0;
    semid->mutex_recurse_count = 0;
    INIT_LIST_HEAD( &semid->pending_tasks );
    INIT_LIST_HEAD( &semid->sem_member_node );
}

int sem_counter_init( semaphore_t *semid, int InitCount )
{
    __sem_init_common( semid );
    semid->u.count = InitCount;
    semid->type  = SEM_TYPE_COUNT;
    return 0;
}

int sem_binary_init( semaphore_t *semid, int InitCount )
{
    __sem_init_common( semid );
    semid->u.count = !!InitCount;
    semid->type  = SEM_TYPE_BINARY;
    return 0;
}

int sem_binary_clear( semaphore_t *semid )
{
	int old;
	
	old = arch_interrupt_disable();
	semid->u.count = 0;
	arch_interrupt_enable(old );
	return 0;
}

int sem_counter_clear( semaphore_t *semid )
{
	return sem_binary_clear(semid);
}

int mutex_init( semaphore_t *semid )
{
    __sem_init_common( semid );
    semid->type                 = SEM_TYPE_MUTEX;
    return 0;
}

int sem_binary_take( semaphore_t *semid, unsigned int tick )
{
    int old;
    int TaskStatus = 0;

#ifndef KERNEL_NO_ARG_CHECK

    if ( semid->type != SEM_TYPE_BINARY ) {
        ptcb_current->err = EINVAL;
        return -1;
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
        return -1;
    }

    if ( tick != WAIT_FOREVER ) {
        ptcb_current->status |= TASK_DELAYD;
        TaskStatus = TASK_DELAYD;
        softtimer_add( &ptcb_current->tick_node, tick );
    }
    
  again:
    READY_Q_REMOVE( ptcb_current );
    ptcb_current->status |= TASK_SUSPEND | TaskStatus;
    
    __put_tcb_to_pendlist( semid, ptcb_current );

    /*
     *  remember which list we are pending on.
     */
    ptcb_current->psem_list      = &semid->pending_tasks;
    
    ptcb_current->err          = 0;
    schedule_internel();
    
    /*
     *  we are not pending on any list now
     */
    ptcb_current->psem_list      = NULL;

    /*
     *  ignore any error if we got the semaphore.
     */
    if ( semid->u.count ) {
        semid->u.count = 0;
        softtimer_remove( &ptcb_current->tick_node );
        arch_interrupt_enable(old );
        return 0;
    }
    
    if ( ptcb_current->err == 0 ) {
        goto again;
    }
    
    if ( ETIME == ptcb_current->err ) {
        /*
         *  time out.
         */
        arch_interrupt_enable(old );
        return -1;
    }
    
    softtimer_remove( &ptcb_current->tick_node );
    arch_interrupt_enable(old );
    return -1;
}

int sem_counter_take( semaphore_t *semid, unsigned int tick )
{
    int old;
    int TaskStatus = 0;

#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(semid->type != SEM_TYPE_COUNT) ) {
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
        return -1;
    }

    if ( tick != WAIT_FOREVER ) {
        ptcb_current->status |= TASK_DELAYD;
        TaskStatus = TASK_DELAYD;
        softtimer_add( &ptcb_current->tick_node, tick );
    }
    
  again:
    READY_Q_REMOVE( ptcb_current );
    ptcb_current->status |= TASK_SUSPEND | TaskStatus;
    
    __put_tcb_to_pendlist( semid, ptcb_current );

    /*
     *  remember which list we are pending on.
     */
    ptcb_current->psem_list      = &semid->pending_tasks;
    
    ptcb_current->err          = 0;
    schedule_internel();
    
    /*
     *  we are not pending on any list now
     */
    ptcb_current->psem_list      = NULL;
    
    /*
     *  ignore any error if we got the semaphore.
     */
    if ( semid->u.count ) {
        semid->u.count--;
        softtimer_remove( &ptcb_current->tick_node );
        arch_interrupt_enable(old );
        return 0;
    }
    
    if ( ptcb_current->err == 0 ) {
        goto again;
    }
    
    if ( ETIME == ptcb_current->err ) {
        /*
         *  time out.
         */
        arch_interrupt_enable(old );
        return -1;
    }
    
    softtimer_remove( &ptcb_current->tick_node );
    arch_interrupt_enable(old );
    return -1;
}

int mutex_lock( semaphore_t *semid, unsigned int tick )
{
    int old;
    int TaskStatus = 0;

#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(IS_INT_CONTEXT()) ) {
        return -1;
    }
    if ( unlikely(semid->type != SEM_TYPE_MUTEX) ) {
        return -1;
    }
#endif
    
    old = arch_interrupt_disable();

    if ( semid->u.owner == NULL ) {
        __mutex_owner_set( semid, ptcb_current );
        semid->mutex_recurse_count++;
        arch_interrupt_enable(old );
        return 0;
    } else if ( semid->u.owner == ptcb_current ) {
        semid->mutex_recurse_count++;
        arch_interrupt_enable(old );
        return 0;
    }
    if ( tick == 0 ) {
        arch_interrupt_enable(old );
        return -1;
    }
    if ( tick != WAIT_FOREVER ) {
        ptcb_current->status |= TASK_DELAYD;
        TaskStatus = TASK_DELAYD;
        softtimer_add( &ptcb_current->tick_node, tick );
    }
  again:
    READY_Q_REMOVE( ptcb_current );
    ptcb_current->status |= TASK_SUSPEND | TaskStatus;

    /*
     *  remember which list we are pending on.
     */
    ptcb_current->psem_list      = &semid->pending_tasks;

    /*
     *  put tcb into pend list and inherit priority.
     */
    __mutex_priority_inheritance( semid );

    ptcb_current->err          = 0;
    schedule_internel();
    
    /*
     *  we are not pending on any list now
     */
    ptcb_current->psem_list      = NULL;
    if ( semid->u.owner == NULL ) {
        __mutex_owner_set( semid, ptcb_current );
        semid->mutex_recurse_count = 1;
        softtimer_remove( &ptcb_current->tick_node );
        arch_interrupt_enable(old );
        return 0;
    }

    if ( ptcb_current->err == 0 ) {
        goto again;
    }

    if ( ETIME == ptcb_current->err ) {
        /*
         *  time out.
         */
        arch_interrupt_enable(old );
        return -2;
    }

    softtimer_remove( &ptcb_current->tick_node );
    arch_interrupt_enable(old );
    return -1;
}

int mutex_unlock( semaphore_t *semid )
{
    int old;
#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(semid->type != SEM_TYPE_MUTEX) ) {
        return -1;
    }
    if ( unlikely(IS_INT_CONTEXT()) ) {
        return -1;
    }
#endif
    old = arch_interrupt_disable();
    if ( semid->u.owner != ptcb_current ) {
        arch_interrupt_enable(old );
        return -1;
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

static void __release_one_mutex( semaphore_t *semid )
{
    tcb_t *ptcbWakeup;
    struct list_head *p;

    if ( list_empty( &semid->pending_tasks ) ) {
        __mutex_owner_set( semid, NULL );
        return ;
    }
    
    p = __sem_pend_list_get_and_remove_first_node( semid );
    ptcbWakeup = PEND_NODE_TO_PTCB( p );
    READY_Q_PUT( ptcbWakeup, ptcbWakeup->current_priority );
    ptcbWakeup->status = TASK_READY;
    __mutex_owner_set( semid, NULL );
}

int sem_binary_give( semaphore_t *semid )
{
    struct list_head *p;
    tcb_t *ptcbWakeup;                                                   /*  the task to wake up         */
    int old;

#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(semid->type != SEM_TYPE_BINARY) ) {
        return -1;
    }
#endif

    old = arch_interrupt_disable();
    semid->u.count = 1;
    if ( list_empty( &semid->pending_tasks ) ) {
        arch_interrupt_enable(old );
        return 0;
    }
    p = __sem_pend_list_get_and_remove_first_node( semid );
    ptcbWakeup = PEND_NODE_TO_PTCB( p );
    READY_Q_PUT( ptcbWakeup, ptcbWakeup->current_priority );
    ptcbWakeup->status = TASK_READY;
    if ( !IS_INT_CONTEXT() ) {
        schedule_internel();
    }
    arch_interrupt_enable(old );
    return 0;
}

int sem_counter_give( semaphore_t *semid )
{
    struct list_head *p;
    tcb_t *ptcbWakeup;                                                   /*  the task to wake up         */
    int old;
    
#ifndef KERNEL_NO_ARG_CHECK
    if ( unlikely(semid->type != SEM_TYPE_COUNT) ) {
        return -1;
    }
#endif
    old = arch_interrupt_disable();
    if ( ++semid->u.count == 0 ) {
        --semid->u.count;
        arch_interrupt_enable(old );
        return -1;
    }
    if ( list_empty( &semid->pending_tasks ) ) {
        arch_interrupt_enable(old );
        return 0;
    }
    p = __sem_pend_list_get_and_remove_first_node( semid );
    ptcbWakeup = PEND_NODE_TO_PTCB( p );
    READY_Q_PUT( ptcbWakeup, ptcbWakeup->current_priority );
    ptcbWakeup->status = TASK_READY;
    if ( !IS_INT_CONTEXT() ) {
        schedule_internel();
    }
    arch_interrupt_enable(old );
    return 0;
}

static inline int __get_pend_list_priority ( semaphore_t *semid )
{
    tcb_t *ptcb;
    
    if ( !list_empty(&semid->pending_tasks) ) {
        ptcb = PEND_NODE_TO_PTCB( LIST_FIRST(&semid->pending_tasks) );
        return ptcb->current_priority;
    }
    return MAX_PRIORITY;
}

static inline int __get_mutex_hold_list_priority ( tcb_t *ptcb )
{
    semaphore_t *semid;
    
    if ( !list_empty(&ptcb->mutex_holded_head) ) {
        semid = SEM_MEMBER_PTR_TO_SEMID( LIST_FIRST(&ptcb->mutex_holded_head) );
        return __get_pend_list_priority(semid);
    }
    return MAX_PRIORITY;
}
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

static int  __mutex_owner_set( semaphore_t *semid, tcb_t *ptcbToAdd )
{
    struct list_head *p;
    semaphore_t      *psem;
    int               pri;

    if ( NULL == ptcbToAdd ) {
        semid->u.owner = NULL;
        list_del_init( &semid->sem_member_node );
        return 0;
    }

    semid->u.owner              = ptcbToAdd;

    pri = __get_pend_list_priority(semid);
    list_for_each( p, &ptcbToAdd->mutex_holded_head) {
        psem = SEM_MEMBER_PTR_TO_SEMID( p );
        if ( __get_pend_list_priority( psem ) > pri )  {
            break;
        }
    }

    list_add_tail( &semid->sem_member_node, p );
    return 0;
}


static inline
struct list_head *__sem_pend_list_get_and_remove_first_node( semaphore_t *semid )
{
    struct list_head *p;
    
    p = LIST_FIRST( &semid->pending_tasks );
    list_del_init( p );
    return p;
}

static int __resort_hold_mutex_list_and_trig( semaphore_t *semid, tcb_t *ptcbOwner )
{
    int pri;
    
    __mutex_owner_set(semid, NULL);
    pri = __get_mutex_hold_list_priority(ptcbOwner);
    __mutex_owner_set(semid, ptcbOwner);
    return pri > __get_mutex_hold_list_priority(ptcbOwner);
}

static int __resort_pend_list_and_trig( semaphore_t *semid, tcb_t *ptcb )
{
    int pri;
    
    pri = __get_pend_list_priority(semid);
    list_del_init( &ptcb->sem_node );
    __put_tcb_to_pendlist( semid, ptcb );
    return pri > __get_pend_list_priority(semid);
}

static int __insert_pend_list_and_trig( semaphore_t *semid, tcb_t *ptcb )
{
    int pri;
    
    pri = __get_pend_list_priority(semid);
    __put_tcb_to_pendlist( semid, ptcb );
    return pri > __get_pend_list_priority(semid);
}

static void __mutex_priority_inheritance( semaphore_t *semid )
{
    tcb_t *pOwner;
    int    priority;
    
    pOwner   = semid->u.owner;
    priority = ptcb_current->current_priority;

    if ( __insert_pend_list_and_trig( semid, ptcb_current ) ) {
      again:
        if ( __resort_hold_mutex_list_and_trig(semid, pOwner) ) {
            if ( pOwner->current_priority > priority ) {
                pOwner->current_priority = priority;
                if ( pOwner->status == TASK_READY ) {
                    READY_Q_REMOVE( pOwner );
                    READY_Q_PUT( pOwner, priority );
                } else if ( pOwner->status & TASK_SUSPEND ) {
                    semid   = PLIST_PTR_TO_SEMID( pOwner->psem_list );
                    if ( __resort_pend_list_and_trig( semid, pOwner ) &&
                         (semid->type == SEM_TYPE_MUTEX) ) {
                        pOwner = semid->u.owner;
                        goto again;
                    }
                }
            }
        } /* if ( __resort_ ... */
    }
}

static void __restore_current_task_priority ( semaphore_t *semid )
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

static void task_exit( void )
{
    
    arch_interrupt_disable();
    READY_Q_REMOVE( ptcb_current );
    list_del_init( &ptcb_current->task_list_node );
    schedule_internel();
    /*
     *  never return
     */
}

int task_stop_remove( tcb_t *ptcb )
{
    int old;
    int ret = -1;
    struct list_head *save, *p;
    
    old = arch_interrupt_disable();
    if ( ptcb->safe_count ) {
        goto done;
    }
    
    /*
     *  remove delay node
     */
    if ( !list_empty( &(ptcb->tick_node.node) )) {
        softtimer_remove( &ptcb->tick_node );
    }
    
    list_for_each_safe(p, save, &ptcb->mutex_holded_head){
        semaphore_t *psemid;
        psemid = SEM_MEMBER_PTR_TO_SEMID( p );
        __release_one_mutex( psemid );
    }
    
    READY_Q_REMOVE( ptcb );
    list_del_init( &ptcb->task_list_node );
    ptcb->status = TASK_DEAD;
    
  done:
    arch_interrupt_enable(old );
    return ret;
}

int task_startup( tcb_t *ptcb )
{
    int old;
    int ret = -1;

    
    old = arch_interrupt_disable();
    if ( !list_empty(&ptcb->task_list_node) ) {
        arch_interrupt_enable(old );
        return -1;
    }

    list_add_tail( &ptcb->task_list_node, &GlistAllTaskListHead );
    READY_Q_PUT( ptcb, ptcb->current_priority );

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
    int len = strlen(name) + 1;
    char *p;

    ptcb->priority         = priority;
    ptcb->current_priority = priority;
    ptcb->status           = TASK_READY;
    ptcb->psem_list        = (void*)0;
    ptcb->option           = option;
    ptcb->stack_low        = stack_low;
    ptcb->stack_high       = stack_high;
    ptcb->safe_count       = 0;

    /*
     *  setting stack name
     */
    p = --stack_high - len;
    stack_high = p - 1;
    if ( p <= stack_low ) {
        return;
    }
    ptcb->name = p;
    while ( (*(p++) = *name++) ){
    }

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

static void schedule_internel( void )
{
	tcb_t *p;

	p = highest_tcb_get();
    if ( p != ptcb_current) {
        if (IS_INT_CONTEXT()) {
            arch_context_switch_interrupt( &ptcb_current->sp, &p->sp);
        } else {
            arch_context_switch( &ptcb_current->sp, &p->sp);        
        }
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
	int old;
	tcb_t *p;

    old = arch_interrupt_disable();
    if ( IS_INT_CONTEXT() ) {
        goto done;
    }
    
	p = highest_tcb_get();
	if ( p != ptcb_current ){
        if (IS_INT_CONTEXT()) {
            arch_context_switch_interrupt( &ptcb_current->sp, &p->sp);
        } else {
            arch_context_switch( &ptcb_current->sp, &p->sp);        
        }
	}
done:
    arch_interrupt_enable(old);
}

static void task_idle( void *arg )
{
    task_safe();
    while (1) {
    	schedule();
    }
}

void kernel_init( void )
{
	TASK_INFO_DECL(static, info1, 512);
	
    INIT_LIST_HEAD( &GlistAllTaskListHead );
    INIT_LIST_HEAD(&GlistDelayHead);
    priority_q_init( &g_readyq );
    TASK_INIT( "idle", info1, MAX_PRIORITY-1, task_idle, 0,0 );
    TASK_STARTUP(info1);
}

unsigned int tick_get( void )
{
    return g_systick;
}


int msgq_init( msgq_t *pmsgq, int objsize, int unit_size )
{
    int     buffer_size;
    int     count;

    buffer_size = objsize - sizeof(msgq_t);
    count = buffer_size / unit_size;
    
    if ( buffer_size == 0 || count == 0 ) {
        return -EINVAL;
    }

    pmsgq->buff_size = buffer_size;
    pmsgq->rd        = 0;
    pmsgq->wr        = 0;
    pmsgq->unit_size = unit_size;
    pmsgq->count     = count;

    sem_counter_init( &pmsgq->sem_rd,  0 );
    sem_counter_init( &pmsgq->sem_wr,  count );

    return 0;
}


/**
 *  @brief recieve msg from a msgQ
 *  @param pmsgq     a pointor to the msgQ.(the return value of function msgq_create)
 *  @param buff      the memory to store the msg. It can be NULL. if
 *                   it is NULL, it just remove one message from the head.
 *  @param buff_size the buffer size.
 *  @param tick      the max time to wait if there is no message.
 *                   if pass -1 to this, it will wait forever.
 *  @param keep      0: copy the msg to the buffer and then remove it.
 *                   !0: copy the msg to the buffer, but do not remove it.
 *  @retval -1       error, please check errno. if errno == ETIME, it means Timer expired,
 *                   if errno == ENOMEM, it mean buffer_size if not enough.
 *  @retval 0        recieve successfully.
 */
int msgq_recieve( msgq_t *pmsgq, void *buff, int buff_size, int tick, int keep )
{
    int ret;
    int rd;
    int old;

    old = arch_interrupt_disable();
    ret = sem_counter_take( &pmsgq->sem_rd, tick );
    if ( ret ) {
        arch_interrupt_enable(old);
        return -ETIME;
    }
    rd = pmsgq->rd;
    if ( !keep ) {
        pmsgq->rd = (pmsgq->rd + 1) % pmsgq->count;
    }
    arch_interrupt_enable(old);

    if ( buff ) {
        memcpy( buff, pmsgq->buff + pmsgq->unit_size*rd, pmsgq->unit_size );
    }

    sem_counter_give( &pmsgq->sem_wr );
    return 0;
}
/**
 *  @brief send message to a the message Q.
 *  @param pmsgq     a pointor to the msgQ.(the return value of function msgq_create)
 *  @param buff      the message to be sent.
 *  @prarm size      the size of the message to be sent in bytes.
 *  @param tick      if the msgQ is not full, this function will return immedately, else it
 *                   will block some tick. Set it to -1 if you want to wait forever.
 *  @retval 0        OK
 *  @retval -EINVAL or -ENODATA or -ETIME      the msgQ is full or pmsgQ is not valid. check errno for details.
 *                   errno:
 *                     -EINVAL - Invalid argument
 *                     -ETIME  - Timer expired
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
    ret = sem_counter_take( &pmsgq->sem_wr, tick );
    if ( ret ) {
        arch_interrupt_enable(old);
        return -ETIME;
    }
    next = (pmsgq->wr + 1) % pmsgq->count;
    wr = pmsgq->wr;
    pmsgq->wr = next;
    
    arch_interrupt_enable(old);

    if ( buff ) {
        memcpy( pmsgq->buff + pmsgq->unit_size*wr, buff, int_min(pmsgq->unit_size, size) );
    }

    sem_counter_give( &pmsgq->sem_rd );
    return 0;
}

int msgq_clear( msgq_t *pmsgq )
{
    int old;

    if ( NULL == pmsgq ) {
        return -1;
    }
    
    old = arch_interrupt_disable();
    pmsgq->sem_wr.u.count = pmsgq->count;
    pmsgq->sem_rd.u.count = 0;
    pmsgq->rd                 = pmsgq->wr = 0;
    arch_interrupt_enable(old);
    return 0;
}

