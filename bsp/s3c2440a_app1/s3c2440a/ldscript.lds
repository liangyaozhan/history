


OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
 	  SDRAM(xrw) : ORIGIN = 0x30000000, LENGTH = 48M
 	  DMARAM(rw) : ORIGIN = 0x33000000, LENGTH = 16M
}


SECTIONS
{
	.text :
	{
    	. = ALIGN(4);
        __local_ram_low_address__ = .;
        KEEP(*(.isr_vector))            /* Startup code */
		*(.text)
		*(.text.*)
		*(.romrun)
	}>SDRAM

	. = ALIGN(4);
	.rodata : { 
	    *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) 
	    } >SDRAM

    . = ALIGN(4);
    .ctors :
    {
        PROVIDE(__ctors_start__ = .);
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
        PROVIDE(__ctors_end__ = .);
    }

    .dtors :
    {
        PROVIDE(__dtors_start__ = .);
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
        PROVIDE(__dtors_end__ = .);
    }

	. = ALIGN(4);
	.data : {
         *(.data) 
  } >SDRAM

	  __exidx_start = .;
	  .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) }
	  __exidx_end = .;
	  
	. = ALIGN(4);
	.got : { *(.got) } >SDRAM

	. = ALIGN(4);
	__bss_start = .;
	.bss (NOLOAD) : { 
	    *(.bss) 
        *(COMMON)
        . = ALIGN(4); 
	
	} >SDRAM
	_end = .;
    
   . = ALIGN(4);
   __sys_heap_start__         = .;
   __sys_heap_end__           = LENGTH(SDRAM) + __local_ram_low_address__;
   __local_ram_high_address__ = LENGTH(SDRAM) + __local_ram_low_address__;

    /* no cache memory area */
    dma_memory_area : {
        lds_no_cache_mem_start = .;
        lds_mmu_table_address = .;
        . += 64K;
        lds_usbmem_address_start = .;
        . += 16K;
        lds_usbmem_address_end = .;
        lds_ohcimem_address_start = .;
        . += 16K;
        lds_ohcimem_address_end = .;
        lds_dma_mem_pool_start  = .;
        . += 64K;
        lds_dma_mem_pool_end    = .;
        lds_lcd_frame_buffer_start = .;
        lds_no_cache_mem_end = LENGTH(DMARAM);
        lds_lcd_frame_buffer_end = lds_no_cache_mem_end;
        . = lds_no_cache_mem_end;
        lds_no_cache_mem_size = LENGTH(DMARAM) - ORIGIN(DMARAM);
    } >DMARAM
}
