/* Last modified Time-stamp: <2012-10-26 22:36:08 Friday by lyzh>
 * @(#)mmu-armarchv4.c
 */

#include "mmu.h"


void arm_mmu_set( unsigned int L1_table_address, unsigned int L2_table_address, mmu_t *pmap, int count)
{
    unsigned int  i;
    unsigned int *pL1;
    mmu_t        *pmmu_tmp;
    unsigned int  size;

    /*
     *  disable all;
     */
    pL1  = (unsigned int*)L1_table_address;
    for (i = 0; i < 4096; i++) {
        /* *pL1++ = (i*0x100000)| (3<<10)|0x12; */
        *pL1++ = (i*0x100000)|0x12;
    }

    for (i = 0; i < count; i++) {
        pmmu_tmp = &pmap[i];
        size = pmmu_tmp->size;
        if ( size >= SIZE_1MB ) {
            int          k;
            int          n;
            unsigned int ap;
            unsigned int cb;
            unsigned int value;
            unsigned int section = 0x12;

            /*
             *  TODO:  
             *  phase AP
             */
            if ( pmmu_tmp->attr & (MMU_WRITE_EN|MMU_READ_EN) ) {
                ap = 3;
            } else {
                ap = 0;
            }

            /*
             * TODO:  
             *  phase cb;
             */
            if ( pmmu_tmp->attr & (MMU_ICACHE_EN|MMU_DCACHE_EN) ) {
                cb = 3;
            } else {
                cb = 0;
            }
            
            n    = size/SIZE_1MB;
            pL1  = (unsigned int*)L1_table_address;
            pL1 += (pmmu_tmp->vraddr>>20);
            for (k = 0; k < n; k++) {
                value  = (pmmu_tmp->phaddr + k*SIZE_1MB) & 0xfff00000;
                value += (ap<<10) | (cb<<2) | section;
                *pL1++ = value;
            }
        } else {
            
        }
    }
}

