/* Last modified Time-stamp: <2012-10-28 08:38:02 Sunday by lyzh>
 * @(#)bsp.c
 */

#include "rtk.h"

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "misc.h"

int os_clk_init( void )
{
    /* Configure the SysTick */
    SysTick_Config( SystemCoreClock / 1000 );
    return 0;
}

void SysTick_Handler(void)
{
	extern void softtimer_announce( void );
	ENTER_INT_CONTEXT();
	softtimer_announce();
	EXIT_INT_CONTEXT();
}

void assert_failed(uint8_t* file, uint32_t line)
{
    kprintf ("assert_failed at %s:%d\n", file, line );
    task_current->safe_count = 0;
    task_terminate( task_current );
}
