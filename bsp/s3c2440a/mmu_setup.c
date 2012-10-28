/* Last modified Time-stamp: <2012-10-27 09:53:30 Saturday by lyzh>
 * @(#)mmu_setup.c
 */


#include "mmu.h"

void arm_mmu_set( unsigned int L1_table_address, unsigned int L2_table_address, mmu_t *pmap, int count);

/*
 *  these symbols is defined in ld script.
 */
extern int lds_no_cache_mem_start;
extern int lds_no_cache_mem_size;


mmu_t mmap_static[] = {
    {
        0x00000000, 0x30000000, SIZE_1MB, MMU_ICACHE_EN|MMU_DCACHE_EN|MMU_EXEC|MMU_WRITE_EN|MMU_READ_EN
    },
    {
        0x30000000, 0x30000000, SIZE_1MB*(64-16), MMU_ICACHE_EN|MMU_DCACHE_EN|MMU_EXEC|MMU_WRITE_EN|MMU_READ_EN
    },
    {
        0x20000000, 0x20000000, SIZE_1MB, MMU_WRITE_EN|MMU_READ_EN
    },
    {
        (int)&lds_no_cache_mem_start,
        (int)&lds_no_cache_mem_start,
        (int)&lds_no_cache_mem_size,
        MMU_WRITE_EN|MMU_READ_EN
    },
    {
        0x48000000, 0x48000000, SIZE_1MB*0x200, MMU_WRITE_EN|MMU_READ_EN
    },
};

void arm_mmu_table_setup( int L1 )
{
    arm_mmu_set( L1, L1+16*1024, mmap_static, sizeof(mmap_static)/sizeof(mmap_static[0]));
}


