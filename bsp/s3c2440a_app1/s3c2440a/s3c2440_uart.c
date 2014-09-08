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
** File name:               s3c2440_uart.c
** Last modified Date:      2012-2-2
** Last Version:            1.0.0
** Descriptions:            S3C2440 UART
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
#include "s3c2440.h"
#include "s3c2440_clock.h"
#include <rtk.h>

void    debugChannelInit (int  iChannelNum);

void uart_init( void )
{
/*
    GPHCON  = (GPHCON & (~(0x0F << 4))) | (0x0A << 4);

    GPHUP  |= 0x0F;

    UFCON0  = 0x00;
    
    UMCON0  = 0x00;

    ULCON0  = 0x03;

    UCON0   = 0x245;
#endif
    /*
     * (int)(UART clock / (baud rate x 16)) �C1
     */
    /* UBRD0   = ((unsigned long)(PCLK / 16.0 / 115200 + 0.5) - 1); */
    
    debugChannelInit(0);
}

/*********************************************************************************************************
  MACRO
*********************************************************************************************************/
#define COM0            0                                               /*  ���� 0                      */
#define COM1            1                                               /*  ���� 1                      */
#define COM2            2                                               /*  ���� 2                      */

#define CHK_ODD         0x4                                             /*  ��У��                      */
#define CHK_EVEN        0x5                                             /*  żУ��                      */
#define CHK_NONE        0x0                                             /*  û��У��                    */

#define USE_INF         1                                               /*  ʹ�ú���ģʽ                */
#define UNUSE_INF       0                                               /*  ��ʹ�ú���ģʽ              */

#define ONE_STOPBIT     0                                               /*  һ��ֹͣλ                  */
#define TWO_STOPBIT     1                                               /*  ����ֹͣλ                  */



#define __COM0_CLKBIT       (1 << 10)                                   /*  COM0 �� CLKCON �е�λ��     */
#define __COM1_CLKBIT       (1 << 11)                                   /*  COM1 �� CLKCON �е�λ��     */
#define __COM2_CLKBIT       (1 << 12)                                   /*  COM2 �� CLKCON �е�λ��     */

#define __COM0_GPIO         ((1 << 2) | (1 << 3))                       /*  COM0 �� IO �����е�λ��     */
#define __COM1_GPIO         ((1 << 4) | (1 << 5))                       /*  COM1 �� IO �����е�λ��     */
#define __COM2_GPIO         ((1 << 6) | (1 << 7))                       /*  COM2 �� IO �����е�λ��     */

#define __COM0_GPHCON       ((0x2 <<  4) | (0x2 <<  6))                 /*  COM0 �� GPHCON �е�����     */
#define __COM1_GPHCON       ((0x2 <<  8) | (0x2 << 10))                 /*  COM1 �� GPHCON �е�����     */
#define __COM2_GPHCON       ((0x2 << 12) | (0x2 << 14))                 /*  COM1 �� GPHCON �е�����     */

#define __COM0_MASK         ((0x3 <<  4) | (0x3 <<  6))                 /*  COM0 �� GPHCON �е�����     */
#define __COM1_MASK         ((0x3 <<  8) | (0x3 << 10))                 /*  COM1 �� GPHCON �е�����     */
#define __COM2_MASK         ((0x3 << 12) | (0x3 << 14))                 /*  COM2 �� GPHCON �е�����     */

#define TXD0READY   (1 << 1)
#define RXD0READY   (1 << 0)

int kputc(unsigned char c);

int kputc( unsigned char  c )
{
    return 0;
}


int poll_uart_putc( int c )
{
    kputc( c );
    return 1;
}

int serial_putc(unsigned char c)
{
    while ( !(USTAT0 & TXD0READY)) {
    }

    UTXH0 = c;
    if ( c == '\n' ) {
    	serial_putc( '\r' );
    }
    return 1;
}

int serial_puts( const char *str )
{
    int c;
    const char *p=str;

    if ( NULL == str ) {
        serial_puts("<NULL>");
        return 0;
    }
    
    while ( (c=*str++)) {
        serial_putc( c );
    }
    return str-p;
}

unsigned char kgetc(void)
{
    while (!(USTAT0 & RXD0READY)) {
        ;
    }
    return URXH0;
}

int uart_try_getc( char *pc )
{
    if ((USTAT0 & RXD0READY)) {
        *pc = URXH0;
        return 1;
    }
    return 0;
}


int     uartInit (int   iCom,
                  int   iInFrared,
                  int   iData,
                  int   iStopBit,
                  int   iCheck,
                  int   iBaud,
                  void *pvIsrRoutine)
{
    unsigned int        uiUBRDIVn;                                      /*  �����ʷ�����ֵ              */
    unsigned int        uiULCONn;                                       /*  �߿�����ֵ                  */
    unsigned int        uiUCONn;                                        /*  ���ڿ��ƼĴ���ֵ            */
    unsigned int        uiUFCONn;                                       /*  FIFO ���ƼĴ���ֵ           */
    
    unsigned int        uiInterEn;                                      /*  �Ƿ������ж�                */
    
    if (iData < 5) {                                                    /*  ���λ�����                */
        return  (-1);
    }
    iData -= 5;                                                         /*  ȷ���Ĵ����е�ֵ            */
    
    uiInterEn = (pvIsrRoutine == (void *)0) ? 0 : 1;                    /*  ȷ���Ƿ���Ҫ�ж�            */
    
    iBaud = (iBaud << 4);                                               /*  baud = baud * 16            */
    iBaud = PCLK / iBaud;
    iBaud = (iBaud - 1);
	
    uiUBRDIVn = iBaud;                                                  /*  ������                      */
    
    uiULCONn  = ((iInFrared << 6)
              |  (iCheck    << 3)
              |  (iStopBit  << 2)
              |  (iData));                                              /*  �������Ϣ                */
    
    uiUCONn   = ((0x00 << 10)                                           /*  PCLK                        */
              |  (1 << 9)                                               /*  Tx Interrupt Type LEVEL     */
              |  (1 << 8)                                               /*  Rx Interrupt Type LEVEL     */
              |  (1 << 7)                                               /*  Rx Time Out Enable          */
              |  (0 << 6)                                               /*  Rx Error Status             */
                                                                        /*  Interrupt Disable           */
              |  (0 << 5)                                               /*  Loopback Mode Disable       */
              |  (0 << 4)
              |  (1 << 2)                                               /*  Tx Interrupt or poll        */
              |  (1));                                                  /*  Rx Interrupt or poll        */
    
    uiUFCONn  = ((0x0 << 6)                                             /*  Tx FIFO Trigger Level 0     */
              |  (0x2 << 4)                                             /*  Rx FIFO Trigger Level 16    */
              |  (1 << 2)                                               /*  Tx FIFO Reset               */
              |  (1 << 1)                                               /*  Rx FIFO Reset               */
              |  (1));                                                  /*  FIFO Enable                 */

    if (iCom == COM0) {                                                 /*  ���� UART0 �� �ܽ�          */

        GPHCON &= ~(__COM0_MASK);
        GPHCON |=   __COM0_GPHCON;
        GPHUP  &= ~(__COM0_GPIO);                                      /*  ʹ����������                */
        
        CLKCON |=   __COM0_CLKBIT;                                     /*  ʱ�ӹҽ�                    */

        UCON0   = 0;
        UFCON0  = uiUFCONn;
        UMCON0  = 0;                                                   /*  �ر�����                    */
        ULCON0  = uiULCONn;
        UCON0   = uiUCONn;
        UBRD0 = uiUBRDIVn;
		
        if (uiInterEn) {                                                /*  �����жϷ�����            */
            /*API_InterVectorConnect(VIC_CHANNEL_UART0, 
                                   (PINT_SVR_ROUTINE)pvIsrRoutine,
                                   LW_NULL,
                                   "uart0_isr");*/
            /*INTER_CLR_MSK((1u << VIC_CHANNEL_UART0));*/                   /*  ��������ж�                */
            /*INTER_CLR_SUBMSK(BIT_SUB_RXD0); */                            /*  �򿪽����ж�                */
        }
    
    } else if (iCom == COM1) {                                          /*  ���� UART1 �� �ܽ�          */
    
        GPHCON &= ~(__COM1_MASK);
        GPHCON |=   __COM1_GPHCON;
        GPHUP  &= ~(__COM1_GPIO);                                      /*  ʹ����������                */
        
        CLKCON |=   __COM1_CLKBIT;                                     /*  ʱ�ӹҽ�                    */
        
        UCON1   = 0;
        UFCON1  = uiUFCONn;
        UMCON1  = 0;                                                   /*  �ر�����                    */
        ULCON1  = uiULCONn;
        UCON1   = uiUCONn;
        UBRD1 = uiUBRDIVn;
        
        if (uiInterEn) {                                                /*  �����жϷ�����            */
            /*API_InterVectorConnect(VIC_CHANNEL_UART1, 
                                   (PINT_SVR_ROUTINE)pvIsrRoutine,
                                   LW_NULL,
                                   "uart1_isr");*/
            /*INTER_CLR_MSK((1u << VIC_CHANNEL_UART1));*/                   /*  ��������ж�                */
            /*INTER_CLR_SUBMSK(BIT_SUB_RXD1); */                            /*  �򿪽����ж�                */
        }
    
    } else if (iCom == COM2) {                                          /*  ���� UART2 �� �ܽ�          */
        
        GPHCON &= ~(__COM2_MASK);
        GPHCON |=   __COM2_GPHCON;
        GPHUP  &= ~(__COM2_GPIO);                                      /*  ʹ����������                */
        
        CLKCON |=   __COM2_CLKBIT;                                     /*  ʱ�ӹҽ�                    */
        
        UCON2   = 0;
        UFCON2  = uiUFCONn;
        UMCON2  = 0;                                                   /*  �ر�����                    */
        ULCON2  = uiULCONn;
        UCON2   = uiUCONn;
        UBRD2 = uiUBRDIVn;
		
        if (uiInterEn) {                                                /*  �����жϷ�����            */
            /*API_InterVectorConnect(VIC_CHANNEL_UART2, 
                                   (PINT_SVR_ROUTINE)pvIsrRoutine,
                                   LW_NULL,
                                   "uart2_isr");*/
            /*INTER_CLR_MSK((1u << VIC_CHANNEL_UART2));*/                   /*  ��������ж�                */
            /*INTER_CLR_SUBMSK(BIT_SUB_RXD2);*/                             /*  �򿪽����ж�                */
        }
    
    } else {                                                            /*  ���ڳ���                    */
        
        return  (-1);
    }
    
    return  (0);
}
int     uartSendByte (int   iCom, unsigned char  ucData)
{
    switch (iCom) {
    
    case COM0:
        while (UFSTAT0 & (1 << 14));
        while (((UFSTAT0) >> 8) & 0x3F);                               /*  Tx_FIFO IS EMPTY            */
        WrUTXH0(ucData);
        break;
		
    case COM1:
        while (UFSTAT1 & (1 << 14));
        while (((UFSTAT1) >> 8) & 0x3F);                               /*  Tx_FIFO IS EMPTY            */
        WrUTXH1(ucData);
        break;
    
    case COM2:
        while (UFSTAT2 & (1 << 14));
        while (((UFSTAT2) >> 8) & 0x3F);                               /*  Tx_FIFO IS EMPTY            */
        WrUTXH2(ucData);
        break;
    
    default:                                                            /*  ���ںŴ���                  */
        return  (1);
    }
    
    return  (0);
}
/*********************************************************************************************************
** Function name:           uartSendByteCnt
** Descriptions:            UART ����ָ�����ȵ����
** input parameters:        iCom                ���ں�
**                          pucData             ��ݻ�����
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void uartSendByteCnt (int             iCom, 
                         unsigned char  *pucData,
                         int             iCnt)
{
    for (; iCnt != 0; iCnt--) {
        uartSendByte(iCom, *pucData);                                   /*  �������                    */
        pucData++;
    }
}
/*********************************************************************************************************
** Function name:           uartSendString
** Descriptions:            UART ����һ���ַ�
** input parameters:        iCom                ���ں�
**                          pcData              �ַ�
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void uartSendString (int   iCom, char  *pcData)
{
    if (!pcData) {                                                      /*  ָ��Ϊ��                    */
        return;
    }
    
    while (*pcData != '\0') {                                           /*  �����ַ�                  */
        uartSendByte(iCom, (unsigned char)*pcData);
        pcData++;
    }
}
/*********************************************************************************************************
** Function name:           debugChannelInit
** Descriptions:            ��ʼ�����Խӿ�
** input parameters:        iChannelNum                 ͨ����
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void    debugChannelInit (int  iChannelNum)
{
    uartInit(iChannelNum, UNUSE_INF, 8, ONE_STOPBIT, CHK_NONE, 115200, (void *)0);
}
