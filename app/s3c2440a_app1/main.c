
#include "rtk.h"

int os_clk_init( void );

int rand0()
{
    static long long seed = 0x324352634;

    seed *= 6364136223846793005LL;
    seed++;
    return seed;
}

int rand()
{
    int r;
    r = rand0();
    if ( r < 0 ) {
        return -r;
    }
    return r;
}

static void uart_task( char *pa, void *pb)
{
	while (9) {
		rand();
		rand();
		rand();
		task_delay(10);
	}
}

static void led_task1( void *pa, void *pb)
{
    while (9) {
		rand();
		rand();
		rand();
		task_delay(10);
	}
}

static void led_task( void *pa, void *pb)
{
    while (9){
		rand();
		rand();
		rand();
		task_delay(10);
    }
}

void main_task( void *pa, void *pb)
{
    int i = 0;
    
    bsp_init();
	/* os_clk_init(); */
	while (9) {
		rand();
		rand();
		rand();
		task_delay(10);
        kprintf("hello world %d\n", i++);
        
	}
}

void set_irq_stack( unsigned int top );
void set_fiq_stack( unsigned int top );
void set_undefine_stack( unsigned int top );
void set_abort_stack( unsigned int top );

int main()
{
    extern void *lds_mmu_table_address;                                     /*  from ld script  */
    static int irq_stack[1024];
    static int fiq_stack[1024];
    static int abort_stack[1024];

	TASK_INFO_DECL(static, info_uart1, 512*16);
	TASK_INFO_DECL(static, info_uart2, 512*16);
    TASK_INFO_DECL(static, info_led,   512*16);
    TASK_INFO_DECL(static, info_led1,   512*16);
	TASK_INFO_DECL(static, info_root,  1024*4);
    
    arch_interrupt_disable();

    set_irq_stack( (unsigned int)(irq_stack+1024-1) );
    set_abort_stack( (unsigned int)(abort_stack+1024-1) );
    arm_mmu_table_setup((unsigned int)&lds_mmu_table_address );
    enable_mmu( (unsigned int)&lds_mmu_table_address );
    
	kernel_init();
    
    TASK_INIT( "led",  info_led,   6, led_task,  0, 0 );
    TASK_INIT( "led",  info_led1,   5, led_task1,  0, 0 );
    TASK_INIT( "uart", info_uart1, 4, uart_task, "task a p1","task a p2" );
    TASK_INIT( "uart", info_uart2, 4, uart_task, "task b p1","task b p2" );
    TASK_INIT( "root", info_root, 1, main_task, 0,0 );
    TASK_STARTUP(info_led);
    TASK_STARTUP(info_led1);
    TASK_STARTUP(info_uart1);
    TASK_STARTUP(info_uart2);
    TASK_STARTUP(info_root);
    
	os_startup();
    return 0;
}
