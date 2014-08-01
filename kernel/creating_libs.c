/* Last modified Time-stamp: <2014-08-01 18:51:58, by lyzh>
 * @(#)creating_libs.c
 */

#include "rtk.h"
#include "rtklib.h"

struct rtk_mutex *mutex_create( void )
{
    struct rtk_mutex *p = (struct rtk_mutex*)malloc(sizeof(struct rtk_mutex));
    if ( p ) {
        mutex_init( p );
    }
    return p;
}

void mutex_delete( struct rtk_mutex *mutex )
{
    if ( mutex ) {
        mutex_terminate( mutex );
        free( mutex );
    }
}

struct rtk_semaphore *semc_create( int init_count )
{
    struct rtk_semaphore *p;

    p = malloc( sizeof(struct rtk_semaphore) );
    if (p) {
        semc_init( p, init_count );
    }
    return p;
}

void semc_delete( struct rtk_semaphore *semid )
{
    if ( semid ) {
        semc_terminate( semid );
        free( semid );
    }
}

struct rtk_semaphore *semb_create( int init_count )
{
    struct rtk_semaphore *p;

    p = malloc( sizeof(struct rtk_semaphore) );
    if (p) {
        semb_init( p, init_count );
    }
    return p;
}

void semb_delete( struct rtk_semaphore *semid )
{
    if ( semid ) {
        semb_terminate( semid );
        free( semid );
    }
}

struct rtk_tcb *task_create(const char *name,
                   int         priority, /* priority of new task */
                   int         stack_size,
                   int         option, /* task option word */
                   void       *pfunc, /* entry point of new task */
                   void       *arg1, /* 1st of 10 req'd args to pass to entryPt */
                   void       *arg2)
{
    struct rtk_tcb *p;
    char  *stack;
    int    len = strlen( name )+1;

    p = malloc( sizeof( struct rtk_tcb ) + stack_size+len );
    stack = (char*)p + sizeof( struct rtk_tcb );
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

int task_delete( struct rtk_tcb *ptcb )
{
    int flag = 1;
    int ret;
    
    if ( ptcb == NULL || ptcb == rtk_self() ) {
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

struct rtk_msgq *msgq_create( int element_size, int element_count )
{
    struct rtk_msgq *p;
    int     size;

    size = element_size*element_count;
    p = (struct rtk_msgq*) malloc( size + sizeof (struct rtk_msgq) );
    if ( p ) {
        msgq_init( p, (char*)p+sizeof(struct rtk_msgq), size, element_size );
    }
    return p;
}

void msgq_delete( struct rtk_msgq *pmsgq )
{
    msgq_terminate( pmsgq );
    free( pmsgq );
}
