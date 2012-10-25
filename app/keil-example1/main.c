
#include "rtk.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "misc.h"

void hw_usart_init( void );
int  serial_putc( char c );
void rt_hw_led_on(int n);
void rt_hw_led_off(int n);
void rt_hw_led_init(void);
int  os_clk_init( void );
void test_uart_isr_init( void );
int  uart_get_char( char *pc, int tick );
int  uart_putchar( char c );
static void led_task( int a, int b );

SEM_BINARY_DECL( GLOBAL, led_sem, 0 );
SEM_BINARY_DECL( GLOBAL, led0_sem, 0 );
TASK_INFO_DECL(static, info_led0,  256);

void main_task( void *pa, void *pb)
{
    os_clk_init();
    hw_usart_init();
    kprintf( "system core clock = %dHz\n", SystemCoreClock );
    
    TASK_INIT( "led", info_led0, 7, led_task, 0, 0 );
    TASK_STARTUP(info_led0);
    task_priority_set( ptcb_current, 7 );
    test_uart_isr_init();
    while (1) {
				rt_hw_led_off(1);
				task_delay( 50 );
				rt_hw_led_on(1);
				task_delay( 50 );
    }
}

static void led_task( int a, int b )
{
    while (1) {
				rt_hw_led_off(0);
				task_delay( 50 );
				rt_hw_led_on(0);
				task_delay( 50 );
    }
}

    

void uart_test_rxtx( void )
{
    char c;
    int i;
    
    while (9) {
        uart_get_char( &c, -1);
        for (i = 0; i < 20; i++) {
            uart_putchar(c+i);
        }
    }
}

void test_uart_isr_init( void )
{
    TASK_INFO_DECL(static, info_1, 256);
    TASK_INIT( "u",  info_1,   5, uart_test_rxtx,  0, 0 );
    TASK_STARTUP(info_1);
}

int main()
{
    TASK_INFO_DECL(static, info_root,  1024);
    
    arch_interrupt_disable();

    rt_hw_led_init();
    rt_hw_led_on(0);

    kernel_init();
    
    TASK_INIT( "root", info_root, 1, main_task, 0, 0 );
    TASK_STARTUP(info_root);
    
    os_startup();
    return 0;
}

