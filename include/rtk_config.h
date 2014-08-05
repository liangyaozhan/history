/* Last modified Time-stamp: <2014-08-05 12:37:11, by lyzh>
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
 * \brief priority number configuration
 */
#define MAX_PRIORITY            255  /*!< must be <= 1023 and >=0 */

/**
 *  \name API configuration macro
 *  @{
 */
#define CONFIG_SEMC_EN    1
#define CONFIG_SEMB_EN    1
#define CONFIG_MUTEX_EN   1
#define CONFIG_MSGQ_EN    1
#define CONFIG_TASK_PRIORITY_SET_EN    1
#define CONFIG_TASK_TERMINATE_EN       1
#define CONFIG_DEAD_LOCK_DETECT_EN     1
#define CONFIG_DEAD_LOCK_SHOW_EN       1
#define CONFIG_TICK_DOWN_COUNTER_EN    1
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

/** @} grp_rtkcfg */


#endif    /* __ARMV6M_INT_USRCFG_H */

/* end of file */

