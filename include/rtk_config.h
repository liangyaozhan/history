/* Last modified Time-stamp: <2012-10-26 05:59:14 Friday by lyzh>
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
 * ��Ҫ�����ȼ���Ŀ��ȡֵ��Χ��1 <= MAX_PRIORITY  <= 1024�� ���õ����ȼ��ǣ�
 *  0�� MAX_PRIORITY-1�������������Ϊ8����ô�������ȼ���Χ��0-7.
 *    
 *
 * \attention ARMv6M ���������1024�����ȼ���
 *
 * \hideinitializer
 */
#define MAX_PRIORITY            8  /*!< must be <= 256 and >=1 */


/** @} */

/** @} grp_rtkcfg */


#endif	/* __ARMV6M_INT_USRCFG_H */

/* end of file */

