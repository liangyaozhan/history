
/****************************************************************************/
.code 32

#if __thumb__
# define FUNC(name) .type name, %function; .thumb_func; name:
# define SET .thumb_set
#else
# define FUNC(name) .type name, %function; name:
# define SET .set
#endif
#define GLOBAL(name) .global name; FUNC(name)
#define SIZE(name) .size name, .-name
#define FUNC_BEGIN(name) GLOBAL(## name)
#define FUNC_END(name) SIZE(## name)
		
/****************************************************************************/

#define NOINT			0xc0
    
/****************************************************************************/    
FUNC_BEGIN(set_irq_stack)
	/* irq */
	mrs	r1,cpsr
	mov r2, r1 /* save old mode to r2 */
	bic	r1,r1,#0x1f
	orr	r1,r1,#0x12
	msr	cpsr,r1
	mov sp, r0
    
    /* switch back to old mode saved in r2 */
	msr	cpsr,r2
	bx lr
FUNC_END(set_irq_stack)
		
		
FUNC_BEGIN(set_fiq_stack)
	/* fiq */
	mrs	r1,cpsr
	mov r2, r1 /* save old mode to r2 */
	bic	r1,r1,#0x1f
	orr	r1,r1,#0x11
	msr	cpsr,r1
	mov sp, r0
	/* switch back to old mode saved in r2 */
	msr	cpsr,r2
	bx lr
FUNC_END(set_fiq_stack)
		
/****************************************************************************/    
FUNC_BEGIN(set_undefine_stack)
		/* undef */
	mrs	r1,cpsr
	mov r2, r1 /* save old mode to r2 */
	bic	r1,r1,#0x1f
	orr	r1,r1,#0x1b
	msr	cpsr,r1
	mov sp, r0
	/* switch back to old mode saved in r2 */
	msr	cpsr,r2
	bx lr
FUNC_END(set_undefine_stack)
    
/****************************************************************************/    		
FUNC_BEGIN(set_abort_stack)
	/* abort */
	mrs	r1,cpsr
	mov r2, r1 /* save old mode to r2 */
	bic	r1,r1,#0x1f
	orr	r1,r1,#0x17
	msr	cpsr,r1
	mov sp, r0
	/* switch back to old mode saved in r2 */
	msr	cpsr,r2
	bx lr
FUNC_END(set_abort_stack)
    
/****************************************************************************/    
.globl 		rt_thread_switch_interrupt_flag
.globl 		rt_interrupt_from_thread
.globl 		rt_interrupt_to_thread
FUNC_BEGIN(_irqhandle)
	stmfd	sp!, {r0-r12,lr}
/*	bl		rt_interrupt_enter */
	ldr r0,_irq_handle
    mov lr, pc
    bx  r0
/*	bl		rt_interrupt_leave */

	/* if rt_thread_switch_interrupt_flag set, jump to _interrupt_thread_switch and don't return */
	ldr		r0, =rt_thread_switch_interrupt_flag
	ldr		r1, [r0]
	cmp		r1, #1
	beq		_interrupt_thread_switch

	ldmfd	sp!, {r0-r12,lr}
	subs	pc, lr, #4
_irq_handle:
    .word irq_handle
FUNC_END(_irqhandle)

		
_interrupt_thread_switch:
	mov		r1,  #0					/* clear rt_thread_switch_interrupt_flag*/
	str		r1,  [r0]

	ldmfd	sp!, {r0-r12,lr}		/* reload saved registers			*/
	stmfd	sp!, {r0-r3}			/* save r0-r3						*/
	mov		r1,  sp
	add		sp,  sp, #16			/* restore sp						*/
	sub		r2,  lr, #4				/* save old task's pc to r2			*/
	
	mrs		r3,  spsr				/* disable interrupt				*/
	orr		r0,  r3, #NOINT
	msr		spsr_c, r0

	ldr		r0,  =.+8				/* switch to interrupted task's stack*/
	movs	pc,  r0

	stmfd	sp!, {r2}				/* push old task's pc				*/
	stmfd	sp!, {r4-r12,lr}		/* push old task's lr,r12-r4		*/
	mov		r4,  r1					/* Special optimised code below		*/
	mov		r5,  r3
	ldmfd	r4!, {r0-r3}
	stmfd	sp!, {r0-r3}			/* push old task's r3-r0			*/
	stmfd	sp!, {r5}				/* push old task's psr				*/
	mrs		r4,  spsr
	stmfd	sp!, {r4}				/* push old task's spsr				*/

	ldr		r4,  =rt_interrupt_from_thread
	ldr		r5,  [r4]
	str		sp,  [r5]				/* store sp in preempted tasks's TCB*/

	ldr	r6,  =rt_interrupt_to_thread
	ldr	r6,  [r6]
	ldr	sp,  [r6]					/* get new task's stack pointer		*/

	ldmfd	sp!, {r4}				/* pop new task's spsr				*/
	msr		SPSR_cxsf, r4
	ldmfd	sp!, {r4}				/* pop new task's psr				*/
	msr		CPSR_cxsf, r4

	ldmfd	sp!, {r0-r12,lr,pc}    /* pop new task's r0-r12,lr & pc	*/

		
/****************************************************************************/    
FUNC_BEGIN(_data_abort)
    stmfd SP!, {R0-R3}

    /*
    * R1 irq stack top, where we store the task context's R0-R3
    * R2 return address
    * irq stack not changed, so SP += 16.
    */
    MOV R1, SP        
    ADD SP, SP, #16
    MOV R2, r14
    SUB R2, R2, #4

    /*
     * restore CPSR from SPSR,
     * and it is now in SVC32 mode.
     */
    MRS R0, SPSR
    MOV R3, R0
    orr R0, R0, #0x80 /* disable irq interrupt */
    MSR CPSR_c, R0

    /*
     * we use svc32's stack to handle irq service.
     */

    /*
	 * this is returning address.
	 */
    PUSH {R2}
	
    /*
	 * save svc's LR
	 */
    PUSH {LR}

    /*
	 * LR can be a temp register now.
	 * SAVE R2 to LR, we will pass LR to handler.
	 */
	MOV LR, R2
    
    /*
     * we reserve 16 bytes for the task's context R0-R3,
     * 
     */
    STMFD SP!, {R0-R3}
    /* SUB SP, SP, #16 */
    
    /*
     * push R0-R3.
     */
    STMFD SP!, {R0-R4}

    /*
     * load the task context's R0-R3
     */
    MOV R4, R1
    LDMFD R4, {R0-R3}
    /*
     * save the task context's to the places where we preseved for it. 
     * NOTE: they are not in a safe place while calling irq_handle,
     * irq might happen again. 
     */
    ADD   SP, SP, #36
    STMFD SP, {R0-R3}
    SUB   SP, SP, #36

    /*
	 * interrupting address.
	 */
    MOV R0,LR
    BL    data_abort

    /*
     * after irq_handle, irq must be disabled.
     */
    
    LDMFD SP!,{R0-R4}
    /*
     * now,
     * R2 is returning address.
     * R3 is CPSR.
     * R1 is irq stack top.
     */

    MSR CPSR, R3
    LDMFD SP!,{R0-R3, LR, PC}
FUNC_END(_data_abort)

/****************************************************************************/
FUNC_BEGIN(_prefetch_abort)
    stmfd SP!, {R0-R3}

    /*
    * R1 irq stack top, where we store the task context's R0-R3
    * R2 return address
    * irq stack not changed, so SP += 16.
    */
    MOV R1, SP        
    ADD SP, SP, #16
    MOV R2, r14
    SUB R2, R2, #4

    /*
     * restore CPSR from SPSR,
     * and it is now in SVC32 mode.
     */
    MRS R0, SPSR
    MOV R3, R0
    orr R0, R0, #0x80 /* disable irq interrupt */
    MSR CPSR_c, R0

    /*
     * we use svc32's stack to handle irq service.
     */

    /*
	 * this is returning address.
	 */
    PUSH {R2}
	
    /*
	 * save svc's LR
	 */
    PUSH {LR}

    /*
	 * LR can be a temp register now.
	 * SAVE R2 to LR, we will pass LR to handler.
	 */
	MOV LR, R2
    
    /*
     * we reserve 16 bytes for the task's context R0-R3,
     * 
     */
    STMFD SP!, {R0-R3}
    /* SUB SP, SP, #16 */
    
    /*
     * push R0-R3.
     */
    STMFD SP!, {R0-R4}

    /*
     * load the task context's R0-R3
     */
    MOV R4, R1
    LDMFD R4, {R0-R3}
    /*
     * save the task context's to the places where we preseved for it. 
     * NOTE: they are not in a safe place while calling irq_handle,
     * irq might happen again. 
     */
    ADD   SP, SP, #36
    STMFD SP, {R0-R3}
    SUB   SP, SP, #36

    /*
	 * interrupting address.
	 */
    MOV R0,LR
    BL    prefetch_abort

    /*
     * after irq_handle, irq must be disabled.
     */
    
    LDMFD SP!,{R0-R4}
    /*
     * now,
     * R2 is returning address.
     * R3 is CPSR.
     * R1 is irq stack top.
     */

    MSR CPSR, R3
    LDMFD SP!,{R0-R3, LR, PC}
FUNC_END(_prefetch_abort)

/****************************************************************************/
FUNC_BEGIN(_undefined_instruction)
        stmfd SP!, {R0-R3}

    /*
    * R1 irq stack top, where we store the task context's R0-R3
    * R2 return address
    * irq stack not changed, so SP += 16.
    */
    MOV R1, SP        
    ADD SP, SP, #16
    MOV R2, r14
    SUB R2, R2, #4

    /*
     * restore CPSR from SPSR,
     * and it is now in SVC32 mode.
     */
    MRS R0, SPSR
    MOV R3, R0
    orr R0, R0, #0x80 /* disable irq interrupt */
    MSR CPSR_c, R0

    /*
     * we use svc32's stack to handle irq service.
     */

    /*
	 * this is returning address.
	 */
    PUSH {R2}
	
    /*
	 * save svc's LR
	 */
    PUSH {LR}

    /*
	 * LR can be a temp register now.
	 * SAVE R2 to LR, we will pass LR to handler.
	 */
	MOV LR, R2
    
    /*
     * we reserve 16 bytes for the task's context R0-R3,
     * 
     */
    STMFD SP!, {R0-R3}
    /* SUB SP, SP, #16 */
    
    /*
     * push R0-R3.
     */
    STMFD SP!, {R0-R4}

    /*
     * load the task context's R0-R3
     */
    MOV R4, R1
    LDMFD R4, {R0-R3}
    /*
     * save the task context's to the places where we preseved for it. 
     * NOTE: they are not in a safe place while calling irq_handle,
     * irq might happen again. 
     */
    ADD   SP, SP, #36
    STMFD SP, {R0-R3}
    SUB   SP, SP, #36

    /*
	 * interrupting address.
	 */
    MOV R0,LR
    BL    undefined_instruction

    /*
     * after irq_handle, irq must be disabled.
     */
    
    LDMFD SP!,{R0-R4}
    /*
     * now,
     * R2 is returning address.
     * R3 is CPSR.
     * R1 is irq stack top.
     */

    MSR CPSR, R3
    LDMFD SP!,{R0-R3, LR, PC}
FUNC_END(_undefined_instruction)
    

/****************************************************************************/
FUNC_BEGIN(_software_interrupt)
        stmfd SP!, {R0-R3}

    /*
    * R1 irq stack top, where we store the task context's R0-R3
    * R2 return address
    * irq stack not changed, so SP += 16.
    */
    MOV R1, SP        
    ADD SP, SP, #16
    MOV R2, r14
    SUB R2, R2, #4

    /*
     * restore CPSR from SPSR,
     * and it is now in SVC32 mode.
     */
    MRS R0, SPSR
    MOV R3, R0
    orr R0, R0, #0x80 /* disable irq interrupt */
    MSR CPSR_c, R0

    /*
     * we use svc32's stack to handle irq service.
     */

    /*
	 * this is returning address.
	 */
    PUSH {R2}
	
    /*
	 * save svc's LR
	 */
    PUSH {LR}

    /*
	 * LR can be a temp register now.
	 * SAVE R2 to LR, we will pass LR to handler.
	 */
	MOV LR, R2
    
    /*
     * we reserve 16 bytes for the task's context R0-R3,
     * 
     */
    STMFD SP!, {R0-R3}
    /* SUB SP, SP, #16 */
    
    /*
     * push R0-R3.
     */
    STMFD SP!, {R0-R4}

    /*
     * load the task context's R0-R3
     */
    MOV R4, R1
    LDMFD R4, {R0-R3}
    /*
     * save the task context's to the places where we preseved for it. 
     * NOTE: they are not in a safe place while calling irq_handle,
     * irq might happen again. 
     */
    ADD   SP, SP, #36
    STMFD SP, {R0-R3}
    SUB   SP, SP, #36

    /*
	 * interrupting address.
	 */
    MOV R0,LR
    BL    software_interrupt

    /*
     * after irq_handle, irq must be disabled.
     */
    
    LDMFD SP!,{R0-R4}
    /*
     * now,
     * R2 is returning address.
     * R3 is CPSR.
     * R1 is irq stack top.
     */

    MSR CPSR, R3
    LDMFD SP!,{R0-R3, LR, PC}
FUNC_END(_software_interrupt)

		
