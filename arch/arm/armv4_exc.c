/* Last modified Time-stamp: <2012-10-29 20:12:40 Monday by lyzh>
 * @(#)armv4_exc.c
 */
#include "rtk.h"


void undefined_instruction(int lr )
{
    kprintf("undefined instruction @ 0x%08X @%s\n", lr-4, CURRENT_TASK_NAME() );
    task_terminate( ptcb_current );
}
void software_interrupt( int lr )
{
    kprintf("software interrupt @ 0x%08X @%s\n", lr-4, CURRENT_TASK_NAME() );
    task_terminate( ptcb_current );
}
void prefetch_abort( int lr )
{
    kprintf("prefetch abort @ 0x%08X @%s\n", lr-4, CURRENT_TASK_NAME() );
    task_terminate( ptcb_current );
}

void data_abort( int lr )
{
    kprintf("data abort @ 0x%08X @%s\n", lr-4, CURRENT_TASK_NAME() );
    task_terminate( ptcb_current );
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

