/* Last modified Time-stamp: <2012-10-23 19:03:29 Tuesday by lyzh>
 * @(#)armv6m_context.c
 */

#include "rtk.h"

/* flag in interrupt handling */
unsigned int rt_interrupt_from_thread, rt_interrupt_to_thread;
unsigned int rt_thread_switch_interrupt_flag;

/**
 * initializes stack of thread
 */
unsigned char *arch_stack_init(void *tentry, void *parameter1, void *parameter2,
                               char *stack_low, char *stack_high, void *texit)
{
	unsigned long *stk;
    
	stk 	 = (unsigned long*)ROUND_DOWN(stack_high, 8);
	*(stk)   = 0x01000000L;					/* PSR */
	*(--stk) = (unsigned long)tentry;		/* entry point, pc */
	*(--stk) = (unsigned long)texit;		/* lr */
	*(--stk) = 0;							/* r12 */
	*(--stk) = 0;							/* r3 */
	*(--stk) = 0;							/* r2 */
	*(--stk) = (unsigned long)parameter2;	/* r1 */
	*(--stk) = (unsigned long)parameter1;	/* r0 : argument */

	*(--stk) = 0;							/* r11 */
	*(--stk) = 0;							/* r10 */
	*(--stk) = 0;							/* r9 */
	*(--stk) = 0;							/* r8 */
	*(--stk) = 0;							/* r7 */
	*(--stk) = 0;							/* r6 */
	*(--stk) = 0;							/* r5 */
	*(--stk) = 0;							/* r4 */

	/* return task's current stack address */
	return (unsigned char *)stk;
}


extern long list_thread(void);
/**
 * fault exception handling
 */
void arch_hard_fault_exception( int **sp )
{
	kprintf("hard fault on thread %s\n", CURRENT_TASK_NAME());
	while (1);
}

/**
 * shutdown CPU
 */
void rt_hw_cpu_shutdown()
{
	kprintf("shutdown...\n");

}

