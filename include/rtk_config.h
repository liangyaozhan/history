

#ifndef __RTK_CONFIG_H
#define __RTK_CONFIG_H

/**
 * \addtogroup grp_rtkcfg
 * @{
 */

/**
 * \brief 最大优先级数目配置
 *
 * 由于CPU资源可能有限，为了避免不必要的RAM消耗，用户可以将此参数设置为实际
 * 需要的优先级数目。取值范围：1 <= MAX_PRIORITY  <= 1024， 可用的优先级是：
 *  0到 MAX_PRIORITY-1。例：如果配置为8，那么可用优先级范围是0-7.
 *    
 *
 * \attention ARMv6M 最多能设置1024级优先级。
 *
 * \hideinitializer
 */
#define MAX_PRIORITY            8  /*!< must be <= 256 and >=1 */


/** @} */

/** @} grp_rtkcfg */


#endif	/* __ARMV6M_INT_USRCFG_H */

/* end of file */

