/* Last modified Time-stamp: <2012-10-21 12:38:30 Sunday by lyzh>
 * @(#)bsp.c
 */

#include "os.h"

#define RT_USING_UART1


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
	extern void softtimer_anounce( void );
	ENTER_INT_CONTEXT();
	softtimer_anounce();
	EXIT_INT_CONTEXT();
}

