;/*
; * File      : armv7m_context_rvds.S
; * Date           Author       Notes
; * 2012-10-25     liangyaozhan first
; */

;/**
; * @addtogroup grp_armv7m
; */
;/*@{*/

NVIC_INT_CTRL   EQU     0xE000ED04               ; interrupt control state register
NVIC_SYSPRI2    EQU     0xE000ED20               ; system priority register (2)
NVIC_PENDSV_PRI EQU     0x00FF0000               ; PendSV priority value (lowest)
NVIC_PENDSVSET  EQU     0x10000000               ; value to trigger PendSV exception

    AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8

    IMPORT rt_thread_switch_interrupt_flag
    IMPORT rt_interrupt_from_thread
    IMPORT rt_interrupt_to_thread

;/*
; * rt_base_t arch_interrupt_disable();
; */
arch_interrupt_disable    PROC
    EXPORT  arch_interrupt_disable
    MRS     r0, PRIMASK
    CPSID   I
    BX      LR
    ENDP

;/*
; * void arch_interrupt_enable(rt_base_t level);
; */
arch_interrupt_enable    PROC
    EXPORT  arch_interrupt_enable
    MSR     PRIMASK, r0
    BX      LR
    ENDP

;/*
; * void arch_context_switch(rt_uint32 from, rt_uint32 to);
; * r0 --> from
; * r1 --> to
; */
arch_context_switch_interrupt
    EXPORT arch_context_switch_interrupt
arch_context_switch    PROC
    EXPORT arch_context_switch

    PUSH    {R4}
	MRS		R4, PRIMASK
	CPSID	I

    ; set rt_thread_switch_interrupt_flag to 1
    LDR     r2, =rt_thread_switch_interrupt_flag
    LDR     r3, [r2]
    CMP     r3, #1
    BEQ     _reswitch
    MOV     r3, #1
    STR     r3, [r2]

    LDR     r2, =rt_interrupt_from_thread   ; set rt_interrupt_from_thread
    STR     r0, [r2]

_reswitch
    LDR     r2, =rt_interrupt_to_thread     ; set rt_interrupt_to_thread
    STR     r1, [r2]

    LDR     r0, =NVIC_INT_CTRL              ; trigger the PendSV exception (causes context switch)
    LDR     r1, =NVIC_PENDSVSET
    STR     r1, [r0]
	CPSIE	I
	
	;/* restore interrupt */
	MSR		PRIMASK, R4
    
    POP     {R4}
    BX      LR
    ENDP

	
    IMPORT set_ptcb_current
; r0 --> swith from thread stack
; r1 --> swith to thread stack
; psr, pc, lr, r12, r3, r2, r1, r0 are pushed into [from] stack
PendSV_Handler   PROC
    EXPORT PendSV_Handler

    ; disable interrupt to protect context switch
    MRS     r2, PRIMASK
    CPSID   I

    ; get rt_thread_switch_interrupt_flag
    LDR     r0, =rt_thread_switch_interrupt_flag
    LDR     r1, [r0]
    CBZ     r1, pendsv_exit         ; pendsv already handled

    ; clear rt_thread_switch_interrupt_flag to 0
    MOV     r1, #0x00
    STR     r1, [r0]

    LDR     r0, =rt_interrupt_from_thread
    LDR     r1, [r0]
    CBZ     r1, swtich_to_thread    ; skip register save at the first time

    MRS     r1, psp                 ; get from thread stack pointer
    STMFD   r1!, {r4 - r11}         ; push r4 - r11 register
    LDR     r0, [r0]
    STR     r1, [r0]                ; update from thread stack pointer

swtich_to_thread
    LDR     r1, =rt_interrupt_to_thread
    LDR     r1, [r1]
	MOV     R0, R1
    LDR     r1, [r1]                ; load thread stack pointer

    LDMFD   r1!, {r4 - r11}         ; pop r4 - r11 register
    MSR     psp, r1                 ; update stack pointer

	 push {LR}   ;/* a safe way to save LR */
	 ;/* set ptcb_current R0 is &ptcb_current->sp */
     BL      set_ptcb_current
     pop {LR}
	
pendsv_exit
    ; restore interrupt
    MSR     PRIMASK, r2

    ORR     lr, lr, #0x04
    BX      lr
    ENDP

;/*
; * void arch_context_switch_to(rt_uint32 to);
; * r0 --> to
; * this fucntion is used to perform the first thread switch
; */
arch_context_switch_to    PROC
    EXPORT arch_context_switch_to
    ; set to thread
    LDR     r1, =rt_interrupt_to_thread
    STR     r0, [r1]

    ; set from thread to 0
    LDR     r1, =rt_interrupt_from_thread
    MOV     r0, #0x0
    STR     r0, [r1]

    ; set interrupt flag to 1
    LDR     r1, =rt_thread_switch_interrupt_flag
    MOV     r0, #1
    STR     r0, [r1]

    ; set the PendSV exception priority
    LDR     r0, =NVIC_SYSPRI2
    LDR     r1, =NVIC_PENDSV_PRI
    LDR.W   r2, [r0,#0x00]       ; read
    ORR     r1,r1,r2             ; modify
    STR     r1, [r0]             ; write-back

    ; trigger the PendSV exception (causes context switch)
    LDR     r0, =NVIC_INT_CTRL
    LDR     r1, =NVIC_PENDSVSET
    STR     r1, [r0]

    ; enable interrupts at processor level
    CPSIE   I

    ; never reach here!
    ENDP

; compatible with old version
arch_interrupt_thread_switch PROC
    EXPORT arch_interrupt_thread_switch
    BX      lr
    NOP
    ENDP

    IMPORT arch_hard_fault_exception
    EXPORT HardFault_Handler
HardFault_Handler    PROC

    ; get current context
    MRS     r0, psp                 ; get fault thread stack pointer
    PUSH    {lr}
    BL      arch_hard_fault_exception
    POP     {lr}

    ORR     lr, lr, #0x04
    BX      lr
    ENDP

    EXPORT rtk_ffs
rtk_ffs    PROC
    rbit r0,r0
	clz  r0,r0
    add  r0, #1
    BX      lr
    ENDP
	
    END
