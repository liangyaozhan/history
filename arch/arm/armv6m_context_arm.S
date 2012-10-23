;/*
; * File      : context_rvds.S
; * This file is part of RT-Thread RTOS
; * COPYRIGHT (C) 2009, RT-Thread Development Team
; *
; * The license and distribution terms for this file may be
; * found in the file LICENSE in this distribution or at
; * http://www.rt-thread.org/license/LICENSE
; *
; * Change Logs:
; * Date           Author       Notes
; * 2009-01-17     Bernard      first version
; */

;/**
; * @addtogroup CORTEX-M3
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
    MOVS     r3, #1
    STR     r3, [r2]

    LDR     r2, =rt_interrupt_from_thread   ; set rt_interrupt_from_thread
    STR     r0, [r2]

_reswitch
    LDR     r2, =rt_interrupt_to_thread     ; set rt_interrupt_to_thread
    STR     r1, [r2]

    LDR     r0, =NVIC_INT_CTRL              ; trigger the PendSV exception (causes context switch)
    LDR     r1, =NVIC_PENDSVSET
    STR     r1, [r0]
	CPSIE   I
	
	;/* restore interrupt */
	MSR		PRIMASK, R4
    
    POP     {R4}
    BX      LR
    ENDP

; r0 --> swith from thread stack
; r1 --> swith to thread stack
; psr, pc, lr, r12, r3, r2, r1, r0 are pushed into [from] stack
PendSV_Handler   PROC
    EXPORT PendSV_Handler
    IMPORT set_ptcb_current
	
    ; disable interrupt to protect context switch
    MRS     r2, PRIMASK
    CPSID   I

    ; get rt_thread_switch_interrupt_flag
    LDR     r0, =rt_thread_switch_interrupt_flag
    LDR     r1, [r0]
	CMP     R1, #0
    BEQ     pendsv_exit         ; pendsv already handled

    ; clear rt_thread_switch_interrupt_flag to 0
    MOVS     r1, #0x00
    STR     r1, [r0]

    LDR     r0, =rt_interrupt_from_thread
    LDR     r1, [r0]
	CMP     R1, #0
    BEQ     swtich_to_thread    ; skip register save at the first time

    MRS     r1, psp                 ; get from thread stack pointer
	
	SUBS    R1, R1, #4*4
	STMEA     r1!, {r4 - r7}         ; push r4 - r7 register
	SUBS    R1, R1, #4*8
	
	MOV     R4, R8
	MOV     R5, R9
	MOV     R6, R10
	MOV     R7, R11
	
	STMEA     r1!, {r4 - r7}         ; push r8 - r11 register
	SUBS    R1, R1, #4*4
	
    LDR     r0, [r0]
    STR     r1, [r0]                ; update from thread stack pointer

swtich_to_thread
    LDR     r1, =rt_interrupt_to_thread
    LDR     r1, [r1]
    MOVS     R0, R1
    LDR     r1, [r1]                ; load thread stack pointer

    LDMFD   r1!, {r4 - r7}         ; pop r8 - r11 register
	MOV     R8, R4
	MOV     R9, R5
	MOV     R10, R6
	MOV     R11, R7
    LDMFD   r1!, {r4 - r7}         ; pop r4 - r7 register
	
	
    MSR     psp, r1                 ; update stack pointer

    MOV  R1, LR ;/* save LR, quick way but, it is not safe if set_ptcb_current function modify R1 */
    push {R1}   ;/* a safe way to save LR */
    ;/* set ptcb_current R0 is &ptcb_current->sp */
    BL      set_ptcb_current
    pop {R1}
    MOV     LR, R1 ; /* restore LR */
    
pendsv_exit
    ; restore interrupt
    MSR     PRIMASK, r2
	
    MOVS    R0, #0x04	
	MOV     R1,lr
    ORRS    R1, R1, R0
    BX      R1
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
    MOVS     r0, #0x0
    STR     r0, [r1]

    ; set interrupt flag to 1
    LDR     r1, =rt_thread_switch_interrupt_flag
    MOVS     r0, #1
    STR     r0, [r1]

    ; set the PendSV exception priority
    LDR     r0, =NVIC_SYSPRI2
    LDR     r1, =NVIC_PENDSV_PRI
    LDRH    r2, [r0,#0x00]       ; read
    ORRS    r1,r1,r2             ; modify
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

    ;IMPORT arch_hard_fault_exception
    EXPORT HardFault_Handler
HardFault_Handler    PROC

    ; get current context
    MRS     r0, psp                 ; get fault thread stack pointer
    PUSH    {lr}
    ;BL      arch_hard_fault_exception
    POP     {R0}
	MOV     lr, R0

    MOVS    R0, #0x04	
	MOV     R1,lr
    ORRS    R1, R1, R0
    BX      R1
    ENDP

    END
