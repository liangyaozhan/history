

#define STATUS_REG_INIT_VALUE   0x53

#define ROUND_DOWN(p, d)        (((int)p - ((d)-1)) & (~(d-1)))
#ifndef ROUND_UP
#define ROUND_UP(x, align)  (((int) (x) + (align - 1)) & ~(align - 1))
#endif


unsigned char *arch_stack_init(void *tentry, void *a, void *b,
                               char *stack_low, char *stack_high, void *texit)
{
    int *p;
    
    p = (int)ROUND_DOWN(stack_high, 8);

    /*
     *  r0-r12, lr, pc
     */
    *p-- = (int)tentry;  /* pc */
    *p-- = (int)texit;  /* lr */
    *p-- = 0xcccccccc;                  /* r12*/
    *p-- = 0xbbbbbbbb;                  /* r11 */
    *p-- = 0xaaaaaaaa;                  /* r10 */
    *p-- = 0x99999999;                  /* r9 */
    *p-- = 0x88888888;                  /* r8 */
    *p-- = 0x77777777;                  /* r7 */
    *p-- = 0x66666666;                  /* r6 */
    *p-- = 0x55555555;                  /* r5 */
    *p-- = 0x44444444;                  /* r4*/
    *p-- = 0x33333333;                  /* r3*/
    *p-- = 0x22222222;                  /* r2*/
    *p-- = b;                  /* r1: arg1 of function task_entry_exit */
    *p-- = a;                  /* r0: arg0 of function task_entry_exit */
    *p = STATUS_REG_INIT_VALUE; /* cpsr */
    /*
     *  full stack pointer
     */
    return p;
}

static const unsigned char __ffs8_table[256] = {
    0, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    6, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    7, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    6, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    8, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    6, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    7, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    6, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
};


int rtk_ffs( register unsigned int x )
{
    register int r=0;
    register unsigned int h = x>>16;
    
    if ( h ) {
        x = h;
        r += 16;
    }
    h = x >> 8;
    if ( h ) {
        x = h;
        r += 8;
    }
    r +=  __ffs8_table[x];
    return r;
}



/*********************************************************************************************************
 **  end of this file.
 ********************************************************************************************************/
