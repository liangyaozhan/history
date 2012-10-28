
#define MB   (0x100000)
		
/*======================================================================
  MMU Control register initial value
======================================================================*/
#define MMUCR_W_ENABLE	 (1<<3)  /* write buffer enable */
#define MMUCR_PROG32	 (1<<4)  /* PROG32 */
#define MMUCR_DATA32	 (1<<5)  /* DATA32 */
#define MMUCR_L_ENABLE	 (1<<6)  /* Late abort on earlier CPUs */
#define MMUCR_ROM	     (1<<9)  /* ROM bit, modifies MMU protections */

#define MMUCR_MODE       MMUCR_ROM /* This MUST remain ROM for shared libraries */
#define MMUCR_ASYNC      (3<<30) /* Asynchronous mode */

#define MMU_INIT_VALUE (MMUCR_PROG32 | MMUCR_DATA32 | MMUCR_L_ENABLE | \
                        MMUCR_MODE | MMUCR_W_ENABLE)
    
/*********************************************************************************************************
  ARM920T cache index
*********************************************************************************************************/
	.equ        ARM920T_ICACHE_OWAY         ,26
	.equ        ARM920T_ICACHE_OSET         ,5
	.equ        ARM920T_ICACHE_NWAY         ,64
	.equ        ARM920T_ICACHE_NSET         ,8
	.equ        ARM920T_DCACHE_OWAY         ,26
	.equ        ARM920T_DCACHE_OSET         ,5
	.equ        ARM920T_DCACHE_NWAY         ,64
	.equ        ARM920T_DCACHE_NSET         ,8

/*
 *************************************************************************
 *
 * vx_sysInit:
 * Init MMU PLL BANKs timing
 *
 * from vx.
 *
 *************************************************************************
 */
.globl enable_mmu
.text
.balign 4
enable_mmu:
    /*
     * Set processor and MMU to known state as follows (we may have not
     * been entered from a reset). We must do this before setting the CPU
     * mode as we must set PROG32/DATA32.
     *
     * MMU Control Register layout.
     *
     * bit
     *  0 M 0 MMU disabled
     *  1 A 0 Address alignment fault disabled, initially
     *  2 C 0 Data cache disabled
     *  3 W 0 Write Buffer disabled
     *  4 P 1 PROG32
     *  5 D 1 DATA32
     *  6 L 1 Should Be One (Late abort on earlier CPUs)
     *  7 B ? Endianness (1 => big)
     *  8 S 0 System bit to zero } Modifies MMU protections, not really
     *  9 R 1 ROM bit to one     } relevant until MMU switched on later.
     * 10 F 0 Should Be Zero
     * 11 Z 0 Should Be Zero (Branch prediction control on 810)
     * 12 I 0 Instruction cache control
     */

    /*
     * Setup MMU Control Register
     */
    mov r1, #MMU_INIT_VALUE                                             /* Defined in mmuArmLib.h       */
    mcr p15, 0, r1, c1, c0, 0                                           /* Write to MMU CR              */

    /*
     * If MMU was on before this, then we'd better hope it was set
     * up for flat translation or there will be problems. The next
     * 2/3 instructions will be fetched "translated" (number depends
     * on CPU).
     * 
     * And we'd better clean and invalidate data cache before we 
     * reinitialize MMU, due to the image of VxWorks may be cached.
     *
     * We would like to discard the contents of the Write-Buffer
     * altogether, but there is no facility to do this. Failing that,
     * we do not want any pending writes to happen at a later stage,
     * so drain the Write-Buffer, i.e. force any pending writes to
     * happen now.
     */
    /*
     * clean and invalidate D-CACHE
     */
    mov r1,#0
    MCR p15, 0, R1, c7, c14, 0

    /* 
     * drain write-buffer           
     */
    mov r1, #0                                                          /* data SBZ                     */
    mcr p15, 0, r1, c7, c10, 4

    /*
     * Flush (invalidate) both I and D caches
     */
    mcr p15, 0, r1, c7, c7, 0                                           /* R1 = 0 from above, data SBZ  */

    /*
     * Set Process ID Register to zero, this effectively disables
     * the process ID remapping feature.
     */
    mov r1, #0
    mcr p15, 0, r1, c13, c0, 0

	/* Disable Vector Int */
	ldr r1, =0x71200000
	ldr r2, =0xFFFFFFFF	                                                /* disable int                  */
	str r2, [r1, #0x14]
	ldr r1, =0x71300000
	str r2, [r1, #0x14]
	mov	ip, lr
	/* r0 from caller. It is mmutable address. */
    bl mmuSetup  /* startup MMU */

	mov	lr, ip
    mov	pc, lr /* return to _start() */		


__arm_mmu_table_setup:
	.word arm_mmu_table_setup
		
/*
 *************************************************************************
 *
 * mmuSetup:
 * Startup MMU
 * 参考 <<ARM嵌入式系统开发>> 484,485页
 *
 * from vx.
 * set page table PTEs & enable mmu function
 *
 * 0x32000000 --|---------------| ------> <L1_PAGE_TABLE_ADDR>
 *              | L1 page table |
 *              |               |
 *              |     16KB      |
 *              |               |
 * 0x32004000 --|---------------| ------> <L2_PAGE_TABLE_ADDR>
 *              | L2 page table |
 *              |      4KB      |
 * 0x32005000 --|---------------|
 *
 *************************************************************************
 */
#define CONFIG_MEM_PAGE_LOW    0x83c00000
#define CONFIG_MEM_START_ADDR  0x80000000
#define CONFIG_MEM_SIZE        0x03000000
#define CONFIG_MEM_END_ADDR    0x83000000
#define CONFIG_EXC_VECTOR      0x00000000
#define L1_PAGE_TABLE_ADDR     CONFIG_MEM_PAGE_LOW 
#define L1_PAGE_TABLE_SIZE     SZ_16K
#define L2_PAGE_TABLE_ADDR     L1_PAGE_TABLE_ADDR + L1_PAGE_TABLE_SIZE
#define L2_PAGE_TABLE_SIZE     SZ_4K

mmuSetup:
/* 
 * Set Translation Table Base 
 */
  		/*ldr     r1, =L1_PAGE_TABLE_ADDR*/
  		mov r1, r0
        mcr     p15, 0, r1, c2, c0, 0                                   /* Set Translation Table Base   */
                                                                        /* Register                     */
#if 0
/* 
 * Invalidate Instruction, Data TLBs 
 */
        mov     r0,  #0
        mcr     p15, 0, r0, c8, c7, 0                                   /* Flush I & D TLBs             */
#endif

/* 
 * Set Domain Access Control Register 
 */
        ldr     r0, =0x1                                                /* Set All 16 domains to manager*/
                                                                        /* access                       */
        mcr     p15, 0, r0, c3, c0, 0                                   /* Set Domain Permissions       */
#if 0
/*
 * Set bit S=1 R=0,so memory is read-only in super mode and unaccess in user mode.
 */
 		mrc 	p15, 0, r0, c1, c0, 0
 		ldr 	r1, =0xfffffcff
 		and 	r0, r0, r1
 		orr		r0, r0, #0x00000100
 		mcr		p15, 0, r0, c1, c0, 0
#endif

/* 
 * Enable MMU 
 */
        mrc     p15, 0, r0, c1, c0, 0                                   /* Read Control Register        */
        orr     r0, r0, #0x00000001                                     /* Enable MMU                   */

        mcr     p15, 0, r0, c1, c0, 0                                   /* Write Back the Control       */
                                                                        /* Register                     */

/* 
 * Enable Instruction Cache 
 */
        mrc     p15, 0, r0, c1, c0, 0                                   /* Read Control Register        */
        orr     r0, r0, #0x1000                                         /* Set I-Cache bit              */
        mcr     p15, 0, r0, c1, c0, 0                                   /* Write Back Control Register  */
        
/*
 * Enable Data Cache 
 */
        mrc     p15, 0, r0, c1, c0, 0                                   /* Read Control Reg             */
        orr     r0, r0, #0x00000004                                     /* Enable Data Cache            */
        mcr     p15, 0, r0, c1, c0, 0                                   /* Write Back                   */
        
		mov     pc,lr

.globl disable_mmu
.text
.balign 4
disable_mmu:
	 push {r0, r1}
    /*
     * clean and invalidate D-CACHE
     */
    mov r1,#0
1:
    mcr p15,0,r1,c7,c10,2
    
    add r1,r1,#1<<ARM920T_DCACHE_OSET
    tst r1,#ARM920T_DCACHE_NSET<<ARM920T_DCACHE_OSET
    beq 1b
    
    bic r1,r1,#ARM920T_DCACHE_NSET<<ARM920T_DCACHE_OSET
    adds r1,r1,#1<<ARM920T_DCACHE_OWAY
   	bcc 1b 
    /* 
     * drain write-buffer           
     */
    mov r1, #0                                                          /* data SBZ                     */
    mcr p15, 0, r1, c7, c10, 4

    /*
     * Flush (invalidate) both I and D caches
     */
    mcr p15, 0, r1, c7, c7, 0                                           /* R1 = 0 from above, data SBZ  */

#if 0		
	/*
	 * flush v4 I/D caches
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */
#endif
		
	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002300	@ clear bits 13, 9:8 (--V- --RS)
	bic	r0, r0, #0x00000087	@ clear bits 7, 2:0 (B--- -CAM)
	orr	r0, r0, #0x00000002	@ set bit 2 (A) Align
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-Cache
	mcr	p15, 0, r0, c1, c0, 0

	pop {r0, r1}
	mov	pc, lr

		
/*==============================================================================
** FILE END
==============================================================================*/

