/* Last modified Time-stamp: <2014-07-29 10:01:48, by lyzh>
 * @(#)armv4_exc.c
 */
#include "rtk.h"

/* flag in interrupt handling */
unsigned int rt_interrupt_from_thread, rt_interrupt_to_thread;
unsigned int rt_thread_switch_interrupt_flag;


void undefined_instruction(int lr )
{
    kprintf("undefined instruction @ 0x%08X @%s\n", lr-4, CURRENT_TASK_NAME() );
    task_terminate( NULL );
}
void software_interrupt( int lr )
{
    kprintf("software interrupt @ 0x%08X @%s\n", lr-4, CURRENT_TASK_NAME() );
    task_terminate( NULL );
}
void prefetch_abort( int lr )
{
    kprintf("prefetch abort @ 0x%08X @%s\n", lr-4, CURRENT_TASK_NAME() );
    task_terminate( NULL );
}

void data_abort( int lr )
{
    kprintf("data abort @ 0x%08X @%s\n", lr-4, CURRENT_TASK_NAME() );
    task_terminate( NULL );
}
void reserved()
{
}
/* void irq_handle() */
/* { */
    
/* } */
void fiq_handle()
{
}

