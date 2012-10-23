

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

