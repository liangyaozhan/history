/* Last modified Time-stamp: <2012-10-23 19:04:01 Tuesday by lyzh>
 * @(#)kprintf.c
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


static MUTEX_DECL(kprintf_mutex);

int kprintf( const char* str, ... )
{
    va_list arp;
    UCHAR c, f, r;
    ULONG val;
    char s[16];
    int i, w,res, cc;

    if ( !IS_INT_CONTEXT() ) {
        mutex_lock( &kprintf_mutex, -1);
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

