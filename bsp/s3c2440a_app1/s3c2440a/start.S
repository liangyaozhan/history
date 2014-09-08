
/*
 *************************************************************************
 *
 * Jump vector table as in table 3.1 in [1]
 *
 *************************************************************************
 */


.globl main
.globl _start
.code 32 
		
.global vectorTable
.global UndefinedInstruction
.global SoftwareInterrupt
.global PrefetchAbort
.global PrefetchAbort
.global DataAbort
.global Reserved
.global _IrqHandle
.global irq_handle
.global FiqHandle
.global os_init_low_level0

  .section  .isr_vector,"a",%progbits
  .type  _start, %object
  .size  _start, .-_start
_start:
vectorTable:    
    ldr pc, dat_reset
    ldr pc, dat_undefinedinstruction
    ldr pc, dat_softwareinterrupt
    ldr pc, dat_prefetchabort
    ldr pc, dat_dataabort
    ldr pc, dat_reserved
    ldr pc, _irqhandle_entry
    ldr pc, dat_fiqhandle

dat_reset:
        .word reset

dat_undefinedinstruction:
        .word _undefined_instruction

dat_softwareinterrupt:
        .word _software_interrupt

dat_prefetchabort:
        .word _prefetch_abort

dat_dataabort:
        .word _data_abort

dat_reserved:
        .word _reserved
_reserved:
    SUBS PC, LR, #4

_irqhandle_entry:
    .word _irqhandle

dat_fiqhandle:
        .word _fiqhandle
_fiqhandle:
    bl fiq_handle
    subs pc, lr, #4
    
/*
 *************************************************************************
 *
 * Startup Code (reset vector)
 *
 * do important init only if we don't start from memory!
 * setup Memory and board specific bits prior to relocation.
 * relocate armboot to ram
 * setup stack
 *
 *************************************************************************
 */

.globl _armboot_start
_armboot_start:
    .word _start

/*
 * These are defined in the board-specific linker script.
 */
.globl _bss_start
_bss_start:
    .word __bss_start
__lds_no_cache_mem_end:
    .word lds_no_cache_mem_end
    
.globl _bss_end
_bss_end:
    .word _end

/*
 * the actual reset code
 */

reset:
    /*
     * set the cpu to SVC32 mode
     */
    mrs    r0,cpsr
    bic    r0,r0,#0x1f
    orr    r0,r0,#0xd3
    msr    cpsr,r0

    /*
     * we do sys-critical inits only at reboot,
     * not when booting from ram!
     */
    bl    cpu_init_crit
    bl    system_clock_init
    /* Set up the stack                            */
stack_setup:
    ldr    sp, __lds_no_cache_mem_end
	sub    sp, sp, #0x07
    and    sp, sp, #0xfffffff0
clear_bss:
    ldr    r0, _bss_start        /* find start of bss segment        */
    ldr    r1, _bss_end        /* stop here                        */
    mov    r2, #0x00000000        /* clear                            */

clbss_l:str    r2, [r0]        /* clear loop...                    */
    add    r0, r0, #4
    cmp    r0, r1
    blt    clbss_l

			/* call C++ constructors of global objects 							*/
/*	ldr 	r0, =__ctors_start__
	ldr 	r1, =__ctors_end__
	
ctor_loop:
	cmp 	r0, r1
	beq 	ctor_end
	ldr 	r2, [r0], #4
	stmfd 	sp!, {r0-r1}
	mov 	lr, pc
	bx 		r2
	ldmfd 	sp!, {r0-r1}
	b		ctor_loop

ctor_end:*/
		
    ldr r0,__main
	bx  r0
__main:
    .word main
/*
 *************************************************************************
 *
 * CPU_init_critical registers
 *
 * setup important registers
 * setup memory timing
 *
 *************************************************************************
 */
cpu_init_crit:
    /*
     * flush v4 I/D caches
     */
    mov    r0, #0
    mcr    p15, 0, r0, c7, c7, 0    /* flush v3/v4 cache */
    mcr    p15, 0, r0, c8, c7, 0    /* flush v4 TLB */

    /*
     * disable MMU stuff and caches
     */
    mrc    p15, 0, r0, c1, c0, 0
    bic    r0, r0, #0x00002300    /* clear bits 13, 9:8 (--V- --RS) */
    bic    r0, r0, #0x00000087    /* clear bits 7, 2:0 (B--- -CAM) */
    orr    r0, r0, #0x00000002    /* set bit 2 (A) Align */
    orr    r0, r0, #0x00001000    /* set bit 12 (I) I-Cache */
    mcr    p15, 0, r0, c1, c0, 0

    /*
     * Go setup Memory and board specific bits prior to relocation.
     */
    mov    pc, lr /* back to my caller */


/*
 * 系统时钟初始化函数
 * S3C2440: FCLK:HCLK:PCLK = 1:4:8(FCLK is 405 MHz)
 */
#define CLKDIVN 0x4C000014
#define CLK_CTL_BASE    0x4C000000
#define MDIV_405    0x7f << 12
#define PSDIV_405   0x21
#define MDIV  MDIV_405
#define PSDIV PSDIV_405
#define CLKDIV 0x5   /* FCLK:HCLK:PCLK = 1:4:8 */

system_clock_init:
    ldr r0, =CLKDIVN
    mov r1, #CLKDIV
    str r1, [r0]
    /*
     * 设置arm920t为异步时钟模式
     *
     * 复位时，arm920t处于快速总线时钟模式，该模式下
     * core和AMBA都由BCLK控制，设置为异步模式后，core
     * 由FCLK控制
     */
    mrc p15, 0, r1, c1, c0, 0
    orr r1, r1, #0xc0000000
    mcr p15, 0, r1, c1, c0, 0

    mov r1, #CLK_CTL_BASE
    mov r2, #MDIV
    add r2, r2, #PSDIV
    str r2, [r1, #0x04]     /* MPLLCON  */

    mov pc, lr

