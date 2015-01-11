/* Last modified Time-stamp: <2012-10-26 05:59:34 Friday by lyzh>
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

#include <limits.h>


/* PREFER_SIZE_OVER_SPEED */

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y)                                                 \
    (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (long) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (long))

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

void *memcpy(void *dst0, const void *src0, int len0)
{
#if 0
    char *dst = (char *) dst0;
    char *src = (char *) src0;

    _PTR save = dst0;

    while (len0--)
    {
        *dst++ = *src++;
    }

    return save;
#else
    char *dst = dst0;
    const char *src = src0;
    long *aligned_dst;
    const long *aligned_src;
    
    /* If the size is small, or either SRC or DST is unaligned,
       then punt into the byte copy loop.  This should be rare.  */
    if (!TOO_SMALL(len0) && !UNALIGNED (src, dst))
    {
        aligned_dst = (long*)dst;
        aligned_src = (long*)src;

        /* Copy 4X long words at a time if possible.  */
        while (len0 >= BIGBLOCKSIZE)
        {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            len0 -= BIGBLOCKSIZE;
        }

        /* Copy one long word at a time if possible.  */
        while (len0 >= LITTLEBLOCKSIZE)
        {
            *aligned_dst++ = *aligned_src++;
            len0 -= LITTLEBLOCKSIZE;
        }

        /* Pick up any residual with a byte copier.  */
        dst = (char*)aligned_dst;
        src = (char*)aligned_src;
    }

    while (len0--)
        *dst++ = *src++;

    return dst0;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

#undef UNALIGNED
#undef TOO_SMALL
#undef LBLOCKSIZE
#define LBLOCKSIZE (sizeof(long))
#define UNALIGNED(X)   ((long)X & (LBLOCKSIZE - 1))
#define TOO_SMALL(LEN) ((LEN) < LBLOCKSIZE)

void *memset(void *m, int c, int n)
{
    char *s = (char *) m;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
    int i;
    unsigned long buffer;
    unsigned long *aligned_addr;
    unsigned int d = c & 0xff;	/* To avoid sign extension, copy C to an
                                   unsigned variable.  */

    while (UNALIGNED (s))
    {
        if (n--)
            *s++ = (char) c;
        else
            return m;
    }

    if (!TOO_SMALL (n))
    {
        /* If we get this far, we know that n is large and s is word-aligned. */
        aligned_addr = (unsigned long *) s;

        /* Store D into each char sized location in BUFFER so that
           we can set large blocks quickly.  */
        buffer = (d << 8) | d;
        buffer |= (buffer << 16);
        for (i = 32; i < LBLOCKSIZE * 8; i <<= 1)
            buffer = (buffer << i) | buffer;

        /* Unroll the loop.  */
        while (n >= LBLOCKSIZE*4)
        {
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            n -= 4*LBLOCKSIZE;
        }

        while (n >= LBLOCKSIZE)
        {
            *aligned_addr++ = buffer;
            n -= LBLOCKSIZE;
        }
        /* Pick up the remainder with a bytewise loop.  */
        s = (char*)aligned_addr;
    }

#endif /* not PREFER_SIZE_OVER_SPEED */

    while (n--)
        *s++ = (char) c;

    return m;
}

#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
/* Nonzero if X (a long int) contains a NULL byte. */
#define DETECTNULL(X) (((X) - 0x0101010101010101) & ~(X) & 0x8080808080808080)
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

#ifndef DETECTNULL
#error long int is not a 32bit or 64bit byte
#endif

int strlen( const char *str)
{
    const char *start = str;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
    unsigned long *aligned_addr;

    /* Align the pointer, so we can search a word at a time.  */
    while (UNALIGNED (str))
    {
        if (!*str)
            return str - start;
        str++;
    }

    /* If the string is word-aligned, we can check for the presence of
       a null in each word-sized block.  */
    aligned_addr = (unsigned long *)str;
    while (!DETECTNULL (*aligned_addr))
        aligned_addr++;

    /* Once a null is detected, we check each byte in that block for a
       precise position of the null.  */
    str = (char *) aligned_addr;

#endif /* not PREFER_SIZE_OVER_SPEED */

    while (*str)
        str++;
    return str - start;
}
