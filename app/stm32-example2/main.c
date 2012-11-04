
#include "rtk.h"


#define RT_USING_UART1

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "misc.h"

void hw_usart_init();
int serial_putc( char c );
void rt_hw_led_on(int n);
void rt_hw_led_off(int n);
void rt_hw_led_init(void);
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

MSGQ_DECL_NO_INIT(static, test_msgq, sizeof(int)*10);

static void uart_task( char *pa, void *pb)
{
    int rx;
    
	while (9) {
        if ( 0 == msgq_receive( (msgq_t*)&test_msgq, &rx, sizeof (int), -1 ) ) {
            kprintf( "uart task %s pri=%d msgq recieve : %d\n", (char*)pa, CURRENT_TASK_PRIORITY(), rx );
        }
	}
}

static void led_task1( void *pa, void *pb)
{
    while (9) {
		rt_hw_led_on(1);
		task_delay(rand()%1000);
		rt_hw_led_off(1);
		task_delay(rand()%1000);
	}
}


static void led_task( void *pa, void *pb)
{
    int i;
    
	rt_hw_led_on(1);

    while (9){
        task_delay(rand()%50);
    }
}

void main_task( void *pa, void *pb)
{
    int i;
    int ret;

    MSGQ_DO_INIT( test_msgq, sizeof(int) );

	os_clk_init();
    
	hw_usart_init();
    
    kprintf( "system core clock = %dHz\n", SystemCoreClock );

    i = 0;
	while (9) {
		rt_hw_led_on(0);
		task_delay(rand()%500);
		rt_hw_led_off(0);
		task_delay(rand()%500);
        
        ++i;
        ret = msgq_send( &test_msgq, &i, sizeof(i), 10 );
        if ( ret == 0 ) {
            kprintf("msgq_send OK pri=%d\n", CURRENT_TASK_PRIORITY());
        } else {
            kprintf("msgq_send failed ret=%d, pri=%d\n", ret,  CURRENT_TASK_PRIORITY());
        }
	}
}

int main()
{
	TASK_INFO_DECL(static, info_uart1, 512*2);
	TASK_INFO_DECL(static, info_uart2, 512*2);
    TASK_INFO_DECL(static, info_led,   512*2);
    TASK_INFO_DECL(static, info_led1,   512*2);
	TASK_INFO_DECL(static, info_root,  1024);
    
    arch_interrupt_disable();

	rt_hw_led_init();
	rt_hw_led_on(0);

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

#define led1_rcc                    RCC_APB2Periph_GPIOA
#define led1_gpio                   GPIOA
#define led1_pin                    (GPIO_Pin_1)

#define led2_rcc                    RCC_APB2Periph_GPIOA
#define led2_gpio                   GPIOA
#define led2_pin                    (GPIO_Pin_4)


void rt_hw_led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(led1_rcc|led2_rcc,ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin   = led1_pin;
    GPIO_Init(led1_gpio, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = led2_pin;
    GPIO_Init(led2_gpio, &GPIO_InitStructure);
}

void rt_hw_led_on(int n)
{
    switch (n)
    {
    case 0:
        GPIO_SetBits(led1_gpio, led1_pin);
        break;
    case 1:
        GPIO_SetBits(led2_gpio, led2_pin);
        break;
    default:
        break;
    }
}

void rt_hw_led_off(int n)
{
    switch (n)
    {
    case 0:
        GPIO_ResetBits(led1_gpio, led1_pin);
        break;
    case 1:
        GPIO_ResetBits(led2_gpio, led2_pin);
        break;
    default:
        break;
    }
}
