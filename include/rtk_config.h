/* Last modified Time-stamp: <2012-11-05 16:06:08 Monday by liangyaozhan>
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
 * \addtogroup grp_rtkcfg
 * @{
 */

/**
 * \brief ������ȼ���Ŀ����
 *
 * ����CPU��Դ�������ޣ�Ϊ�˱��ⲻ��Ҫ��RAM���ģ��û����Խ��˲�������Ϊʵ��
 * ��Ҫ�����ȼ���Ŀ��ȡֵ��Χ��1 <= MAX_PRIORITY  <= 1023�� ���õ����ȼ��ǣ�
 *  0�� MAX_PRIORITY�������������Ϊ7����ô�������ȼ���Χ��0-7.
 *    
 *
 *
 * \hideinitializer
 */
#define MAX_PRIORITY            255  /*!< must be <= 1023 and >=0 */

#define CONFIG_SEMC_EN    1
#define CONFIG_SEMB_EN    1
#define CONFIG_MUTEX_EN   1
#define CONFIG_MSGQ_EN    1
#define CONFIG_TASK_PRIORITY_SET_EN    1
#define CONFIG_TASK_TERMINATE_EN       1
#define CONFIG_DEAD_LOCK_DETECT_EN     1
#define CONFIG_DEAD_LOCK_SHOW_EN       0

#define IDLE_TASK_STACK_SIZE    1024



/** @} */

/** @} grp_rtkcfg */


#endif    /* __ARMV6M_INT_USRCFG_H */

/* end of file */

