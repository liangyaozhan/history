

.equ DISABLE_IRQ,0x80
.equ DISABLE_FIQ,0x40
.equ SYS_MOD,0x1f
.equ IRQ_MOD,0x12
.equ FIQ_MOD,0x11
.equ SVC_MOD,0x13
.equ ABT_MOD,0x17
.equ UND_MOD,0x1b
.equ MOD_MASK,0x1f


.macro CHANGE_TO_SVC
        msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|SVC_MOD)
.endm

.macro CHANGE_TO_IRQ
        msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|IRQ_MOD)
.endm

.macro CHANGE_TO_SYS
        msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|SYS_MOD)
.endm

    
/***
	* arch_contex_switch( void **sp_from, void **sp_to );
	* R0 ==> sp_from.
	* R1 ==> sp_to.
	*/
    .globl arch_context_switch
    .globl arch_context_switch_interrupt
	.type arch_context_switch, %function    
	.type arch_context_switch_interrupt, %function    
arch_context_switch_interrupt:  
arch_context_switch:
    /* save PC */
	push { lr }
    /* save R14, R12 - R0 */
	push { R0-R12, lr }

    /* save SPSR */
	mrs r2, cpsr
	push {r2}
    
	orr r2, r2, #0x80
	msr cpsr, r2          /* disable irq */
    
	/* save sp */
	mov r2, sp       /* r2 = sp */
	str r2, [r0]       /* R0 = &sp */

    
    /* R1 == &sp */
	/* R2 == sp  */
	ldr r2, [r1]
	mov sp, r2

    /*
	 *  R0 is &sp
	 */
    MOV R0, R1
	bl set_ptcb_current
    
    /* restore SPSR */
	pop {r0}
	msr cpsr, r0

    /* restore R0-R12, R14, R15 */
	LDMFD SP!, {R0-R12, lr, pc}

/***
	* arch_contex_switch_to( void **sp_to );
	* R0 ==> sp_to
	*/
    .globl arch_context_switch_to
    .type arch_context_switch, %function    
arch_context_switch_to:
    /* R0 == &sp */
	/* R1 == sp  */
	ldr r1, [r0]
	mov sp, r1

	bl set_ptcb_current
    
    /* restore SPSR */
	pop {r0}
	msr cpsr, r0

    /* restore R0-R12, R14, R15 */
	LDMFD SP!, {R0-R12, lr, pc}

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
    
.global arch_interrupt_disable
  .func   arch_interrupt_disable
arch_interrupt_disable:
1:	   
   MRS 		R0 , CPSR
   ORR 		R1 , R0, #0xC0
   MSR 		CPSR_c,R1
   MRS 		R2 , CPSR
   AND 		R2 , R2, #0xC0
   CMP 		R2 , #0xC0
   BNE 		1b
   BX  		LR
   .endfunc

    
.global arch_interrupt_enable
   .func   arch_interrupt_enable
arch_interrupt_enable:
   MSR 		CPSR_c,R0
   BX  		LR
  .endfunc		

