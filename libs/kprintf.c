/* Last modified Time-stamp: <2014-08-04 12:54:12, by lyzh>
 * 
 * Copyright (C) 2012 liangyaozhan <ivws02@gmail.com>
 * 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "rtk.h"
#include <stdarg.h>

int serial_putc( char c );
int serial_puts(const char *str);

#undef f_putc
#undef f_puts

#define f_putc(c, f)  serial_putc(c)
#define f_puts(s, f)  serial_puts(s)
#define ULONG           unsigned long
#define UCHAR           unsigned char
#ifndef EOF
#define EOF             -1
#endif


static MUTEX_DEF( kprintf_mutex);

int kprintf( const char* str, ... )
{
    va_list arp;
    UCHAR c, f, r;
    ULONG val;
    char s[16];
    int i, w,res, cc;

    if ( !IS_INT_CONTEXT() ) {
        mutex_lock( &kprintf_mutex, WAIT_FOREVER);
    }
    
    
    va_start(arp, str);

    for (cc = res = 0; cc != EOF; res += cc) {
        c = *str++;
        if (c == 0) break;            /* End of string */
        if (c != '%') {                /* Non escape cahracter */
            cc = f_putc(c, fil);
            if (cc != EOF) cc = 1;
            continue;
        }
        w = f = 0;
        c = *str++;
        if (c == '0') {                /* Flag: '0' padding */
            f = 1; c = *str++;
        }
        while (c >= '0' && c <= '9') {    /* Precision */
            w = w * 10 + (c - '0');
            c = *str++;
        }
        if (c == 'l') {                /* Prefix: Size is long int */
            f |= 2; c = *str++;
        }
        if (c == 's') {                /* Type is string */
            cc = f_puts(va_arg(arp, char*), fil);
            continue;
        }
        if (c == 'c') {                /* Type is character */
            cc = f_putc(va_arg(arp, int), fil);
            if (cc != EOF) cc = 1;
            continue;
        }
        r = 0;
        if (c == 'd') r = 10;        /* Type is signed decimal */
        if (c == 'u') r = 10;        /* Type is unsigned decimal */
        if (c == 'X' || c == 'x' ) r = 16;        /* Type is unsigned hexdecimal */
        if (r == 0) break;            /* Unknown type */
        if (f & 2) {                /* Get the value */
            val = (ULONG)va_arg(arp, long);
        } else {
            val = (c == 'd') ? (ULONG)(long)va_arg(arp, int) : (ULONG)va_arg(arp, unsigned int);
        }
        /* Put numeral string */
        if (c == 'd') {
            if (val & 0x80000000) {
                val = 0 - val;
                f |= 4;
            }
        }
        i = sizeof(s) - 1; s[i] = 0;
        do {
            c = (UCHAR)(val % r + '0');
            if (c > '9') c += 7;
            s[--i] = c;
            val /= r;
        } while (i && val);
        if (i && (f & 4)) s[--i] = '-';
        w = sizeof(s) - 1 - w;
        while (i && i > w) s[--i] = (f & 1) ? '0' : ' ';
        cc = f_puts(&s[i], fil);
    }

    va_end(arp);
	
    if ( !IS_INT_CONTEXT() ) {
        mutex_unlock( &kprintf_mutex );
    }
    
    return (cc == EOF) ? cc : res;
}


#undef f_putc
#undef f_puts

static int vputc( char **b, char c)
{
	char *p = *b;
	*p++ = c;
	*b = p;
	*p = 0;
	return 1;
}
static int vputs( char **b, const char *str )
{
	char *p = *b;
	int n;
	while ( (*p++=*str++)) {
	}
	n = --p - *b;
	*b = p;
	return n;
}

#define f_putc(c, f)  vputc(&buff, c)
#define f_puts(s, f)  vputs(&buff, s)
#define ULONG           unsigned long
#define UCHAR           unsigned char
#ifndef EOF
#define EOF             -1
#endif

int rtk_sprintf( char *buff, const char* str, ... )
{
    va_list arp;
    UCHAR c, f, r;
    ULONG val;
    char s[16];
    int i, w,res, cc;

    va_start(arp, str);

    for (cc = res = 0; cc != EOF; res += cc) {
        c = *str++;
        if (c == 0) break;            /* End of string */
        if (c != '%') {                /* Non escape cahracter */
            cc = f_putc(c, fil);
            if (cc != EOF) cc = 1;
            continue;
        }
        w = f = 0;
        c = *str++;
        if (c == '0') {                /* Flag: '0' padding */
            f = 1; c = *str++;
        }
        while (c >= '0' && c <= '9') {    /* Precision */
            w = w * 10 + (c - '0');
            c = *str++;
        }
        if (c == 'l') {                /* Prefix: Size is long int */
            f |= 2; c = *str++;
        }
        if (c == 's') {                /* Type is string */
            cc = f_puts(va_arg(arp, char*), fil);
            continue;
        }
        if (c == 'c') {                /* Type is character */
            cc = f_putc(va_arg(arp, int), fil);
            if (cc != EOF) cc = 1;
            continue;
        }
        r = 0;
        if (c == 'd') r = 10;        /* Type is signed decimal */
        if (c == 'u') r = 10;        /* Type is unsigned decimal */
        if (c == 'X' || c == 'x' ) r = 16;        /* Type is unsigned hexdecimal */
        if (r == 0) break;            /* Unknown type */
        if (f & 2) {                /* Get the value */
            val = (ULONG)va_arg(arp, long);
        } else {
            val = (c == 'd') ? (ULONG)(long)va_arg(arp, int) : (ULONG)va_arg(arp, unsigned int);
        }
        /* Put numeral string */
        if (c == 'd') {
            if (val & 0x80000000) {
                val = 0 - val;
                f |= 4;
            }
        }
        i = sizeof(s) - 1; s[i] = 0;
        do {
            c = (UCHAR)(val % r + '0');
            if (c > '9') c += 7;
            s[--i] = c;
            val /= r;
        } while (i && val);
        if (i && (f & 4)) s[--i] = '-';
        w = sizeof(s) - 1 - w;
        while (i && i > w) s[--i] = (f & 1) ? '0' : ' ';
        cc = f_puts(&s[i], fil);
    }

    va_end(arp);

    return (cc == EOF) ? cc : res;
}

