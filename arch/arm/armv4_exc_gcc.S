
/****************************************************************************/
.global set_irq_stack
.global set_fiq_stack
.global set_undefine_stack
.global set_abort_stack

.global _irqhandle
.global _data_abort
.global _prefetch_abort
.global _undefined_instruction
.global _software_interrupt
/****************************************************************************/

    
/****************************************************************************/    
set_irq_stack:
	/* irq */
	mrs	r1,cpsr
	mov r2, r1 /* save old mode to r2 */
	bic	r1,r1,#0x1f
	orr	r1,r1,#0x12
	msr	cpsr,r1
	mov sp, r0
    
    /* switch back to old mode saved in r2 */
	msr	cpsr,r2
	mov pc, lr

		
set_fiq_stack:
	/* fiq */
	mrs	r1,cpsr
	mov r2, r1 /* save old mode to r2 */
	bic	r1,r1,#0x1f
	orr	r1,r1,#0x11
	msr	cpsr,r1
	mov sp, r0
	/* switch back to old mode saved in r2 */
	msr	cpsr,r2
	mov pc, lr
    
/****************************************************************************/    
set_undefine_stack:
		/* undef */
	mrs	r1,cpsr
	mov r2, r1 /* save old mode to r2 */
	bic	r1,r1,#0x1f
	orr	r1,r1,#0x1b
	msr	cpsr,r1
	mov sp, r0
	/* switch back to old mode saved in r2 */
	msr	cpsr,r2
	mov pc, lr
    
/****************************************************************************/    		
set_abort_stack:
	/* abort */
	mrs	r1,cpsr
	mov r2, r1 /* save old mode to r2 */
	bic	r1,r1,#0x1f
	orr	r1,r1,#0x17
	msr	cpsr,r1
	mov sp, r0
	/* switch back to old mode saved in r2 */
	msr	cpsr,r2
	mov pc, lr
    
/****************************************************************************/    
_irqhandle:
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
    BL    irq_handle

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


/****************************************************************************/    
_data_abort:
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

/****************************************************************************/
_prefetch_abort:
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


/****************************************************************************/
_undefined_instruction: 
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

    

/****************************************************************************/
_software_interrupt: 
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

