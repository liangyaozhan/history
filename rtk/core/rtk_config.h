/* Last modified Time-stamp: <2014-08-10 12:46:55, by lyzh>
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


#ifndef __RTK_CONFIG_H
#define __RTK_CONFIG_H

/**
 * \addtogroup grp_rtkcfg configuration
 * @{
 */


/**
 *  \name API configuration macro
 *  @{
 */
#ifndef CONFIG_SEMC_EN
#define CONFIG_SEMC_EN    1
#endif

#ifndef CONFIG_SEMB_EN
#define CONFIG_SEMB_EN    1
#endif

#ifndef CONFIG_MUTEX_EN
#define CONFIG_MUTEX_EN   1
#endif

#ifndef CONFIG_MSGQ_EN
#define CONFIG_MSGQ_EN    1
#endif

#ifndef CONFIG_TASK_PRIORITY_SET_EN
#define CONFIG_TASK_PRIORITY_SET_EN    1
#endif

#ifndef CONFIG_TASK_TERMINATE_EN
#define CONFIG_TASK_TERMINATE_EN       1
#endif

#ifndef CONFIG_DEAD_LOCK_DETECT_EN
#define CONFIG_DEAD_LOCK_DETECT_EN     1
#endif

#ifndef CONFIG_DEAD_LOCK_SHOW_EN
#define CONFIG_DEAD_LOCK_SHOW_EN       1
#endif

#ifndef CONFIG_TICK_DOWN_COUNTER_EN
#define CONFIG_TICK_DOWN_COUNTER_EN    1
#endif

/** @} */

/**
 *  \brief idle task stack size
 *
 *  maybe it is defined in makefile.
 */
#ifndef IDLE_TASK_STACK_SIZE
#define IDLE_TASK_STACK_SIZE    1024
#endif

/** @} */


#if DEBUG>0
#define ASSERT(condiction)                                              \
    do{                                                                 \
        if ( (condiction) )                                             \
            break;                                                      \
        kprintf("ASSERT " #condiction "failed: " __FILE__ ":%d: " ": " "\r\n", __LINE__); \
        while (9);                                                      \
    }while (0)
#define KERNEL_ARG_CHECK_EN 1
#else
#define ASSERT(condiction)  do{}while(0)
#define KERNEL_ARG_CHECK_EN 0
#endif
int  kprintf ( const char* str, ... );

/** @} grp_rtkcfg */


#endif    /* __ARMV6M_INT_USRCFG_H */

/* end of file */

