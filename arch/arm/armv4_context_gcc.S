
#define NOINT			0xc0

/*
 * rt_base_t rt_hw_interrupt_disable();
 */
.globl arch_interrupt_disable
arch_interrupt_disable:
	mrs r0, cpsr
	orr r1, r0, #NOINT
	msr cpsr_c, r1
	mov pc, lr

/*
 * void arch_interrupt_enable(rt_base_t level);
 */
.globl arch_interrupt_enable
arch_interrupt_enable:
	msr cpsr, r0
	mov pc, lr

/*
 * void arch_context_switch(rt_uint32 from, rt_uint32 to);
 * r0 --> from
 * r1 --> to
 */
.globl arch_context_switch
arch_context_switch:
	stmfd	sp!, {lr}		@ push pc (lr should be pushed in place of PC)
	stmfd	sp!, {r0-r12, lr}	@ push lr & register file

	mrs	r4, cpsr
	stmfd	sp!, {r4}		@ push cpsr
	mrs	r4, spsr
	stmfd	sp!, {r4}		@ push spsr

	str	sp, [r0]		@ store sp in preempted tasks TCB
	ldr	sp, [r1]		@ get new task stack pointer

	ldmfd	sp!, {r4}		@ pop new task spsr
	msr	spsr_cxsf, r4
	ldmfd	sp!, {r4}		@ pop new task cpsr
	msr	spsr_cxsf, r4

	ldmfd	sp!, {r0-r12, lr, pc}^	@ pop new task r0-r12, lr & pc

/*
 * void arch_context_switch_to(rt_uint32 to);
 * r0 --> to
 */
.globl arch_context_switch_to
arch_context_switch_to:
	ldr	sp, [r0]		@ get new task stack pointer

	ldmfd	sp!, {r4}		@ pop new task spsr
	msr	spsr_cxsf, r4
	ldmfd	sp!, {r4}		@ pop new task cpsr
	msr	cpsr_cxsf, r4

	ldmfd	sp!, {r0-r12, lr, pc}	@ pop new task r0-r12, lr & pc

/*
 * void arch_context_switch_interrupt(rt_uint32 from, rt_uint32 to);
 */
.globl rt_thread_switch_interrupt_flag
.globl rt_interrupt_from_thread
.globl rt_interrupt_to_thread
.globl arch_context_switch_interrupt
arch_context_switch_interrupt:
	ldr r2, =rt_thread_switch_interrupt_flag
	ldr r3, [r2]
	cmp r3, #1
	beq _reswitch
	mov r3, #1				@ set rt_thread_switch_interrupt_flag to 1
	str r3, [r2]
	ldr r2, =rt_interrupt_from_thread	@ set rt_interrupt_from_thread
	str r0, [r2]
_reswitch:
	ldr r2, =rt_interrupt_to_thread		@ set rt_interrupt_to_thread
	str r1, [r2]
	mov pc, lr

		
.globl rtk_fork
.type rtk_fork, %function    
rtk_fork:
		push { lr } /* PC -> LR -> R12 -> R0 -> CPSR */
		mov r1, r0  /* 参数：priority */
		sub r0,r0,r0  /* 子任务的返回值:0 */
		push { R0-R12, lr }
		mrs r0, cpsr
		push {r0}
		
		mov r0, sp       /* r0 = sp */

		/*
		 *  pass R0 and R1 as parameters here,
		 *  R0 is sp	
		 *  R1 is priority from the caller
		 */
		bl rtkfork_croutine
		
		pop {r1}  /* CPSR */
        pop {r1}  /* R0   */
		/* mov R0, R0  相当于:return ppfor_croutine(sp); */
		LDMFD SP!, {R1-R12, lr, pc}
    
