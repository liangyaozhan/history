/* Last modified Time-stamp: <2012-10-30 08:04:45 Tuesday by lyzh>
 * @(#)creating_libs.c
 */

#include "rtk.h"
#include "rtklib.h"

mutex_t *mutex_create( void )
{
    mutex_t *p = (mutex_t*)malloc(sizeof(mutex_t));
    if ( p ) {
        mutex_init( p );
    }
    return p;
}

void mutex_delete( mutex_t *mutex )
{
    if ( mutex ) {
        mutex_terminate( mutex );
        free( mutex );
    }
}

semaphore_t *semc_create( int init_count )
{
    semaphore_t *p;

    p = malloc( sizeof(semaphore_t) );
    if (p) {
        semc_init( p, init_count );
    }
    return p;
}

void semc_delete( semaphore_t *semid )
{
    if ( semid ) {
        semc_terminate( semid );
        free( semid );
    }
}

semaphore_t *semb_create( int init_count )
{
    semaphore_t *p;

    p = malloc( sizeof(semaphore_t) );
    if (p) {
        semb_init( p, init_count );
    }
    return p;
}

void semb_delete( semaphore_t *semid )
{
    if ( semid ) {
        semb_terminate( semid );
        free( semid );
    }
}

tcb_t *task_create(const char *name,
                   int         priority, /* priority of new task */
                   int         stack_size,
                   int         option, /* task option word */
                   void       *pfunc, /* entry point of new task */
                   void       *arg1, /* 1st of 10 req'd args to pass to entryPt */
                   void       *arg2)
{
    tcb_t *p;
    char  *stack;
    int    len = strlen( name )+1;

    p = malloc( sizeof( tcb_t ) + stack_size+len );
    stack = (char*)p + sizeof( tcb_t );
    if ( !p ) {
        goto err_done;
    }
    memcpy( stack+stack_size, name, len );
    name = stack+stack_size;
    task_init( p, name, priority, option, stack, stack+stack_size, pfunc, arg1, arg2 );
    return p;
    
err_done:
    if (p) {
        free(p);
    }
    return NULL;
}

int task_delete( tcb_t *ptcb )
{
    int flag = 1;
    int ret;
    
    if ( ptcb == NULL || ptcb == ptcb_current ) {
        /*
         *  TODO: how to free memory?
         */
        flag = 0;
    }
    ret = task_terminate( ptcb );
    if ( flag ) {
        free( ptcb );
    }
    return ret;
}

msgq_t *msgq_create( int element_size, int element_count )
{
    msgq_t *p;
    int     size;

    size = element_size*element_count;
    p = (msgq_t*) malloc( size + sizeof (msgq_t) );
    if ( p ) {
        msgq_init( p, (char*)p+sizeof(msgq_t), size, element_size );
    }
    return p;
}

void msgq_delete( msgq_t *pmsgq )
{
    msgq_terminate( pmsgq );
    free( pmsgq );
}
