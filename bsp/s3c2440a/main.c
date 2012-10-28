/* Last modified Time-stamp: <2012-10-27 09:54:01 Saturday by lyzh>
 * @(#)main.c
 */


unsigned char *arch_stack_init(void *tentry, void *a, void *b,
                               char *stack_low, char *stack_high, void *texit);
void arch_context_switch( void **spf, void **spt );
void arch_context_switch_to( void **spt );

int count;

int stacka[ 64 ];
int stackb[ 64 ];

int spa;
int spb;

void taska( int a, int b, int c )
{
    while (1) {
        count++;
        arch_context_switch( &spa, &spb );
    }
}

void taskb( int a, int b, int c )
{
    while (1) {
        count++;
        arch_context_switch( &spb, &spa );
    }
}

void exit_point( )
{
    while (1) {
        count++;
    }
}

void starting_switch_test( void )
{
    spa = (int)arch_stack_init( taska, 0x11111111, 0x22222222, stacka, stacka+64 - 1, exit_point );
    spb = (int)arch_stack_init( taskb, 0x11111111, 0x22222222, stackb, stackb+64 - 1, exit_point );
    arch_context_switch_to( &spa );
}


int main( void )
{
    int i;

    starting_switch_test();
    
    /* arm_mmu_table_setup( 0x30000000 + 0x100000 ); */
    /* enable_mmu( 0x30000000 + 0x100000 ); */
    /* while (1) { */
    /*     *(int*)0x30000001 = 1; */
    /*     i = count; */
    /* } */
    return 1;
}

