/* Last modified Time-stamp: <2012-10-24 20:43:26 Wednesday by lyzh>
 * @(#)led.c
 */


#include "rtk.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "misc.h"


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

