/*********************************************************************************************************
**
** Copyright (c) 2011 - 2012  Jiao JinXing <JiaoJinXing1987@gmail.com>
**
** Licensed under the Academic Free License version 2.1
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
**--------------------------------------------------------------------------------------------------------
** File name:               s3c2440_int.c
** Last modified Date:      2012-2-2
** Last Version:            1.0.0
** Descriptions:            S3C2440 �ж�
**
**--------------------------------------------------------------------------------------------------------
** Created by:              JiaoJinXing
** Created date:            2012-2-2
** Version:                 1.0.0
** Descriptions:            �����ļ�
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Descriptions:
**
*********************************************************************************************************/
#include <stdint.h>
#include "rtk.h"
#include "s3c2440.h"
#include "s3c2440_int.h"

#ifndef NULL
#define NULL ((void*)0)
#endif
#define INTERRUPT_NR      INTGLOBAL

/*
 * �жϷ������������
 */
static isr_t  isr_table[INTERRUPT_NR];
static void  *isr_arg_table[INTERRUPT_NR];

int timer4_isr(uint32_t interrupt, void *arg);
/*
 * IRQ �жϴ������
 */
void irq_handle(void)
{
    uint32_t interrupt;
    isr_t isr;

    /*
     * ��չ�Ķ�:
     * http://hi.baidu.com/ch314156/blog/item/027cdd62feb97f3caa184c29.html
     */

    ENTER_INT_CONTEXT();                                                  /*  �����ж�                    */

    interrupt = INTOFFSET;                                              /*  ����жϺ�                  */

    if (interrupt >= INTERRUPT_NR) {
        return;
    }

    isr = isr_table[interrupt];                                         /*  �����жϷ������            */
    isr(isr_arg_table[interrupt]);

    SRCPND  = 1 << interrupt;                                           /*  ����ж�Դ�ȴ�              */

    INTPND  = INTPND;                                                   /*  ����жϵȴ�                */

    EXIT_INT_CONTEXT();                                                   /*  �˳��ж�                    */
}

/*
 * ��Ч�жϷ������
 */
int isr_invaild(int interrupt)
{
    kprintf("invaild interrupt %d!\n", interrupt);

    return -1;
}

/*
 * ��ʼ���ж�
 */
void interrupt_init(void)
{
    int i;

    SRCPND      = 0x00;                                                 /*  ��������ж�Դ�ȴ�          */

    SUBSRCPND   = 0x00;                                                 /*  ����������ж�Դ�ȴ�        */

    INTMOD      = 0x00;                                                 /*  ���������ж�ģʽΪ IRQ      */

    INTMSK      = BIT_ALLMSK;                                           /*  ���������ж�                */

    INTSUBMSK   = BIT_SUB_ALLMSK;                                       /*  �����������ж�              */

    INTPND      = INTPND;                                               /*  ��������жϵȴ�            */

    for (i = 0; i < INTERRUPT_NR; i++) {                                /*  ��ʼ���жϷ������������*/
        isr_table[i]     = (isr_t)isr_invaild;
        isr_arg_table[i] = i;
    }
}

/*
 * �����ж�
 */
void int_disable(uint32_t interrupt)
{
    if (interrupt < INTERRUPT_NR) {
        INTMSK |= 1 << interrupt;
    }
}

/*
 * ȡ�������ж�
 */
void int_enable(uint32_t interrupt)
{
    if (interrupt < INTERRUPT_NR) {
        INTMSK &= ~(1 << interrupt);
    }
}

/*
 * ��װ�жϷ������
 */
void int_connect(int interrupt, isr_t new_isr, void *arg)
{
    if (interrupt < INTERRUPT_NR) {
        if (new_isr != NULL) {
            isr_table[interrupt]     = new_isr;
            isr_arg_table[interrupt] = arg;
        } else {
            isr_table[interrupt]     = (isr_t)isr_invaild;
            isr_arg_table[interrupt] = interrupt;
        }
    }
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
