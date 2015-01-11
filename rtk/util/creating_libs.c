/* Last modified Time-stamp: <2014-08-06 08:20:02, by lyzh>
 * @(#)creating_libs.c
 */

#include <stdio.h>
#include <string.h>
#include "rtk.h"
#include "rtklib.h"

struct rtk_mutex *mutex_create( void )
{
    struct rtk_mutex *p = (struct rtk_mutex*)rtk_malloc(sizeof(struct rtk_mutex));
    if ( p ) {
        mutex_init( p );
    }
    return p;
}

void mutex_delete( struct rtk_mutex *mutex )
{
    if ( mutex ) {
        mutex_terminate( mutex );
        rtk_free( mutex );
    }
}

struct rtk_semaphore *semc_create( int init_count )
{
    struct rtk_semaphore *p;

    p = rtk_malloc( sizeof(struct rtk_semaphore) );
    if (p) {
        semc_init( p, init_count );
    }
    return p;
}

void semc_delete( struct rtk_semaphore *semid )
{
    if ( semid ) {
        semc_terminate( semid );
        rtk_free( semid );
    }
}

struct rtk_semaphore *semb_create( int init_count )
{
    struct rtk_semaphore *p;

    p = rtk_malloc( sizeof(struct rtk_semaphore) );
    if (p) {
        semb_init( p, init_count );
    }
    return p;
}

void semb_delete( struct rtk_semaphore *semid )
{
    if ( semid ) {
        semb_terminate( semid );
        rtk_free( semid );
    }
}

struct rtk_task *task_create(const char *name,
                   int         priority, /* priority of new task */
                   int         stack_size,
                   int         option, /* task option word */
                   void       *pfunc, /* entry point of new task */
                   void       *arg1, /* 1st of 10 req'd args to pass to entryPt */
                   void       *arg2)
{
    struct rtk_task *p;
    char           *stack;
    unsigned int    len = strlen( name )+1;

    p = rtk_malloc( sizeof( struct rtk_task ) + stack_size+len );
    stack = (char*)p + sizeof( struct rtk_task );
    if ( !p ) {
        goto err_done;
    }
    memcpy( stack+stack_size, name, len );
    name = stack+stack_size;
    task_init( p, name, priority, option, stack, stack+stack_size, pfunc, arg1, arg2 );
    return p;
    
err_done:
    if (p) {
        rtk_free(p);
    }
    return NULL;
}

struct rtk_task *task_like( void *pfunc,
                           void *arg,
                           int   priority )
{
    char new_name[ 64 ];
    int len = strlen( task_self()->name );
    if ( len > 32 )
    {
        len = 32;
    }
    memcpy( new_name, task_self()->name, len );
    rtk_sprintf( new_name+len, "%u", tick_get() );
    return task_create( new_name,
                        task_self()->priority,
                        task_self()->stack_high - task_self()->stack_low+1,
                        task_self()->option,
                        pfunc, arg, 0 );
}

int task_delete( struct rtk_task *task )
{
    int flag = 1;
    int ret;
    
    if ( task == NULL || task == task_self() ) {
        /*
         *  TODO: how to rtk_free memory?
         */
        flag = 0;
    }
    ret = task_terminate( task );
    if ( flag ) {
        rtk_free( task );
    }
    return ret;
}

struct rtk_msgq *msgq_create( int element_size, int element_count )
{
    struct rtk_msgq *p;
    int     size;

    size = element_size*element_count;
    p = (struct rtk_msgq*) rtk_malloc( size + sizeof (struct rtk_msgq) );
    if ( p ) {
        msgq_init( p, (char*)p+sizeof(struct rtk_msgq), size, element_size );
    }
    return p;
}

void msgq_delete( struct rtk_msgq *pmsgq )
{
    msgq_terminate( pmsgq );
    rtk_free( pmsgq );
}


struct rtk_tick *rtk_tick_down_counter_create(void)
{
    struct rtk_tick *_this;
    _this = rtk_malloc( sizeof( struct rtk_tick ) );
    if ( _this )
    {
        rtk_tick_down_counter_init(_this);
    }
    return _this;
}

void rtk_tick_down_counter_delete(struct rtk_tick *_this)
{
    rtk_tick_down_counter_stop( _this );
    rtk_free( _this );
}

