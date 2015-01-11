/* Last modified Time-stamp: <2014-07-29 10:15:31, by lyzh>
 * @(#)rtkfork.c
 */

#include "rtk.h"


struct rtk_task *rtkfork_croutine( int *sp, int priority )
{
#if 0
    int size;
    void *spBase;
    struct rtk_task *task;
    int old;
    size    = task_current->StackSize;
    spBase  = (void*)malloc( size );
    if ( NULL == spBase) {
        return NULL;
    }
    task = (struct rtk_task*)malloc( sizeof(struct rtk_task) );
    if ( NULL == task ) {
        free( spBase );
        return NULL;
    }

    /*
     *  init tcb struct
     */
    memcpy( task, task_current, sizeof(*task));
    task->RunningPriority = task->priority = priority;
    INIT_LIST_HEAD( &task->prioNode.node );
    INIT_LIST_HEAD( &task->TickNode.node );
    INIT_LIST_HEAD( &task->pendNode );
    INIT_LIST_HEAD( &task->MutexHoldHead );
    INIT_LIST_HEAD( &task->taskListNode );
    _REENT_INIT_PTR( &task->reent );
    CHECK_INIT( &task->reent );

    task->vBase      = spBase;
    task->name       = spBase;
    task->ulTickUsed = 0;
    task->err        = 0;
    task->handle     = handle_alloc( g_pidlib, task, HANDLE_TYPE_PID );

    /*
     *  copy the hold stack
     */
    memcpy( task->vBase, task_current->vBase, task_current->StackSize);

    /*
     *  set the right stack pointor. This pointor is from the caller.
     */
    task->sp = (STACK_TYPE *)((char*)task->vBase +
                              ((char*)sp-(char*)task_current->vBase));
    
    old = KERNEL_ENTER_CRITICAL();
    list_add_tail( &task->taskListNode, &GlistAllTaskListHead );
    READY_Q_PUT( task, priority );
    KERNEL_EXIT_CRITICAL( old );
    
    KERNEL_EXIT();

    return task;
#else
    return 0;
    #endif
}

