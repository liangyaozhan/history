/* Last modified Time-stamp: <2012-10-27 09:50:51 Saturday by lyzh>
 * @(#)rtkfork.c
 */

#include "rtk.h"


tcb_t *rtkfork_croutine( int *sp, int priority )
{
#if 0
    int size;
    void *spBase;
    tcb_t *ptcb;
    int old;
    size    = ptcb_current->StackSize;
    spBase  = (void*)malloc( size );
    if ( NULL == spBase) {
        return NULL;
    }
    ptcb = (tcb_t*)malloc( sizeof(tcb_t) );
    if ( NULL == ptcb ) {
        free( spBase );
        return NULL;
    }

    /*
     *  init tcb struct
     */
    memcpy( ptcb, ptcb_current, sizeof(*ptcb));
    ptcb->RunningPriority = ptcb->priority = priority;
    INIT_LIST_HEAD( &ptcb->prioNode.node );
    INIT_LIST_HEAD( &ptcb->TickNode.node );
    INIT_LIST_HEAD( &ptcb->pendNode );
    INIT_LIST_HEAD( &ptcb->MutexHoldHead );
    INIT_LIST_HEAD( &ptcb->taskListNode );
    _REENT_INIT_PTR( &ptcb->reent );
    CHECK_INIT( &ptcb->reent );

    ptcb->vBase      = spBase;
    ptcb->name       = spBase;
    ptcb->ulTickUsed = 0;
    ptcb->err        = 0;
    ptcb->handle     = handle_alloc( g_pidlib, ptcb, HANDLE_TYPE_PID );

    /*
     *  copy the hold stack
     */
    memcpy( ptcb->vBase, ptcb_current->vBase, ptcb_current->StackSize);

    /*
     *  set the right stack pointor. This pointor is from the caller.
     */
    ptcb->sp = (STACK_TYPE *)((char*)ptcb->vBase +
                              ((char*)sp-(char*)ptcb_current->vBase));
    
    old = KERNEL_ENTER_CRITICAL();
    list_add_tail( &ptcb->taskListNode, &GlistAllTaskListHead );
    READY_Q_PUT( ptcb, priority );
    KERNEL_EXIT_CRITICAL( old );
    
    KERNEL_EXIT();

    return ptcb;
#else
    return 0;
    #endif
}

