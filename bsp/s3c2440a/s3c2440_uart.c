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
** Descriptions:            创建文件
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

#define UART_BAUD_RATE  115200
#if 0
#ifndef SUBSRCPND
#define SUBSRCPND   (*(volatile int*)0x4a000018)
#endif
#define INTSUBMASK  *((volatile int*)0x4a00001c)

<<<<<<< HEAD
=======
/* semaphore_t *sem_uart_rx; */
/* semaphore_t *sem_uart_tx; */
/* fifo_t *uart_fifo_rx; */
/* fifo_t *uart_fifo_tx; */

>>>>>>> 518b112... newlib-1.20
void uart0_isr( int );
#endif

/*
 * 初始化 UART
 */
void uart_init(void)
{
    GPHCON  = (GPHCON & (~(0x0F << 4))) | (0x0A << 4);

    GPHUP  |= 0x0F;

    UFCON0  = 0x00;
    
    UMCON0  = 0x00;

    ULCON0  = 0x03;

    UCON0   = 0x245;

    /*
     * (int)(UART clock / (baud rate x 16)) –1
     */
    UBRD0   = ((uint32_t)(PCLK / 16.0 / UART_BAUD_RATE + 0.5) - 1);
    
    /* int_connect( 28, uart0_isr, 0); */
    /* sem_uart_rx         = sem_binary_create( 0, 0 ); */
    /* sem_uart_tx         = sem_binary_create( 63, 0 ); */
    /* uart_fifo_rx        = malloc(sizeof(fifo_t)+128); */
    /* uart_fifo_rx->r     = 0; */
    /* uart_fifo_rx->w     = 0; */
    /* uart_fifo_rx->mask  = 128-1; */
    /* uart_fifo_rx->count = 128; */
    /* uart_fifo_tx        = malloc(sizeof(fifo_t)+512); */
    /* uart_fifo_tx->r     = 0; */
    /* uart_fifo_tx->w     = 0; */
    /* uart_fifo_tx->mask  = 128-1; */
    /* uart_fifo_tx->count = 128; */

    /*
     *  level trig interrupt
     */
    /* UCON0 |= (1<<8) | (1<<9) | (1<<7); */

    /* int_enable( 28 ); */
    
    /* INTSUBMASK &= ~3; */
}

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
    while (!(USTAT0 & TXD0READY)) {
        ;
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

void uart0_isr( int v)
{
    char c;
    int change = 0;

    /*
     *  while ( rx fifo count != 0 )
     */
    change = 0;
    while ( UFSTAT0 & 0x1f ) {
        c = URXH0;
        /* FifoWrite( uart_fifo_rx, c); */
        change++;
    }
    if ( change ) {
        /* sem_give( sem_uart_rx ); */
        /*
         *  clear RX
         */
        SUBSRCPND |= 0<<1;
    }
    
    /*
     *  while (fifo not full) {}
     */
    change=0;
    while (!(UFSTAT0 & (1<<14))) {
        change++;
        /* if ( FifoRead( uart_fifo_tx, &c ) ) { */
        /*     UTXH0 = c; */
        /* } else { */
        /*     sem_give( sem_uart_tx ); */
        /*     break; */
        /* } */
    }
    
    if (change) {
        /*
         *  clear TX
         */
        SUBSRCPND |= 1<<1;
    }
}

