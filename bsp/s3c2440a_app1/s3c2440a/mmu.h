/* mmu.h --- mmulib
 */

#ifndef INCLUDED_MMU_H
#define INCLUDED_MMU_H 1


struct __mmu_t
{
    unsigned int vraddr;
    unsigned int phaddr;

#define SIZE_1KB    1024
#define SIZE_4KB    4096
#define SIZE_1MB    0x100000
    unsigned int size;

#define MMU_ICACHE_EN   0x01
#define MMU_DCACHE_EN   0x02
#define MMU_WRITE_EN    0x04
#define MMU_READ_EN     0x08
#define MMU_EXEC        0x10
    unsigned int attr;
};
typedef struct __mmu_t mmu_t;

void arm_mmu_set( unsigned int L1_table_address, unsigned int L2_table_address, mmu_t *pmap, int count);


#endif /* INCLUDED_MMU_H */

