
#include "rtk.h"
#include "rtklib.h"

int os_clk_init( void );

int rand0()
{
    static long long seed = 0x324352634;

    seed *= 6364136223846793005LL;
    seed++;
    return seed;
}

int rand()
{
    int r;
    r = rand0();
    if ( r < 0 ) {
        return -r;
    }
    return r;
}

static void uart_task( char *pa, void *pb)
{
    while (9) {
        rand();
        rand();
        rand();
        task_delay(10);
        /* __asm volatile ("swi 0"); */
    }
}

static void led_task1( void *pa, void *pb)
{
    int i = 0;
    
    while (9) {
        rand();
        rand();
        rand();
        task_delay(10);
        if ( ++i > 20 ) {
            /*
             *  data abort here.
             *  0x4000000 is protected by mmu. So, data abort.
             */
            /* *(volatile int*)0x4000000 = 0; */
        }
    }
}

#define MAX_T 1000
struct rtk_tcb *tcbs[MAX_T];
#define MAX_M 50
struct rtk_mutex mutexs[1000];
struct rtk_semaphore sem;

static void swap( int *a, int *b)
{
    int c = *a;
    *a = *b;
    *b = c;
}
//#define mutex_lock(f,r) 0
//#define mutex_unlock(f)
//#define mutex_init(f)
//#define kprintf(...)
void mtask( void )
{
    int i;
    uint32_t total;
    uint32_t used;
    uint32_t max_used;
    struct rtk_mutex *pm[MAX_M];
    int order[MAX_M];
    int n;
    int size;
    int x, y;
    extern void memory_info(uint32_t *total, uint32_t *used, uint32_t *max_used);
    char *p;
    int ret;
    
    while (1) {
        task_priority_set(  tcbs[rand()%MAX_T], rand()%(MAX_PRIORITY-1) );
        size = rand()%100000;
        for (i=0; i<MAX_M; i++) {
            order[i] = i;
        }
        n = rand()%MAX_M;
        for (i=0; i<n; i++) {
            x = rand()%n;
            y = rand()%n;
            swap( &order[x], &order[y] );
        }
        for (i=0; i<n; i++ ) {
            pm[i] = &mutexs[ rand()%(sizeof(mutexs)/sizeof(mutexs[0])) ];
            ret = mutex_lock( pm[i], /*rand()%100 + 100*/-1 );
            if ( ret ) {
                kprintf("%s: (%d)@%d mutex_lock error: ret=%d\n",CURRENT_TASK_NAME(), rtk_self()->priority, rtk_self()->current_priority, ret );
            } else {
                kprintf("%s: (%d)@%d mutex_lock OK\n", CURRENT_TASK_NAME(), rtk_self()->priority, rtk_self()->current_priority);
            }
        }
        //p = malloc( size );
        //if ( p ) {
        //    memset(p, 0xff, size );
        //}
        //kprintf("%s ...p=0x%X\n", CURRENT_TASK_NAME(), p );
        task_delay( rand() % 50 );
        for (i=0; i<n; i++ ){
            mutex_unlock( pm[order[n-1-i]] );
        }
        memory_info( &total, &used, &max_used  );
        kprintf("%s : (%d) running at %d malloc info: %d(%X) %d %d\n",CURRENT_TASK_NAME(), rtk_self()->priority, rtk_self()->current_priority, total, total, used, max_used );
        //if ( p ) {
        //    free(p);
        //}
    }
}

static void led_task( void *pa, void *pb)
{
    while (9){
        rand();
        rand();
        rand();
        task_delay(10);
        /*
         *  data abort here.
         */
        /* *(volatile int *)-1 = 0; */
    }
}

struct timer_obj
{
    struct rtk_tick base;
    char *name;
    int period;
};

static void timer_callback( struct timer_obj *_this )
{
    kprintf(
        "\n***********************************************\n"
        "\n***********************************************\n"
        "\n**************** timer callback ***************\n"
        "\n***************** 0x%08X ******************\n"
        "\n %s \n"
        "\n***********************************************\n",
        _this,_this->name
        );
    rtk_tick_down_counter_add( &_this->base, _this->period );
}

int rtk_sprintf( char *buff, const char* str, ... );
void * fb_open( void );
void *fb_back_get( void );void fb_flip( void );

void main_task( void *pa, void *pb)
{
    struct rtk_tcb      *ptcb;
    extern int  __sys_heap_start__;
    extern int  __sys_heap_end__;
    int         i = 0;
    char name[32];
    int priority;
    unsigned short *frame_buffer;
    int x;
    int y;
    unsigned short color;
    struct timer_obj timer0 = {
        {},
        "This is named timer0",
        100
    };

    rtk_tick_down_counter_init( &timer0 );
    rtk_tick_down_counter_set_func( &timer0, timer_callback );
    rtk_tick_down_counter_add( &timer0, timer0.period );
    bsp_init();
    system_heap_init( &__sys_heap_start__, &__sys_heap_end__ );

    /* os_clk_init(); */
    kprintf("\r\nsizeof tcb=%d\n", sizeof(struct rtk_tcb));

    for (i=0; i<sizeof(mutexs)/sizeof(mutexs[0]); i++) {
        mutex_init( &mutexs[i] );
    }
    semb_init( &sem, 0 );

    for (i = 0; i < MAX_T; i++) {
        priority = rand()%(MAX_PRIORITY-1);
        rtk_sprintf( name, "t%d-%d", i, priority );
        ptcb = task_create(name, priority, 1024*32, 0, mtask, 1,2 );
        tcbs[i] = ptcb;
    }

    task_delay( 500 );
    frame_buffer = fb_open();

    for (i = 0; i < MAX_T; i++) {
        task_startup( tcbs[i] );
    }
    semb_terminate( &sem );
    task_priority_set( NULL, MAX_PRIORITY );
    while (9) {
        color = rand();
        frame_buffer = fb_back_get();
        for (x=0; x<480; x++) {
            for (y=0; y<600; y++) {
                *(frame_buffer+x+y*240) = color;
            }
        }
        fb_flip();
        //task_delay( 100 );
    }
}

void set_irq_stack( unsigned int top );
void set_fiq_stack( unsigned int top );
void set_undefine_stack( unsigned int top );
void set_abort_stack( unsigned int top );

int main()
{
    extern void *lds_mmu_table_address;                                     /*  from ld script  */
    static int irq_stack[1024];
    static int fiq_stack[1024];
    static int abort_stack[1024];

    static TASK_INFO_DEF( info_uart1, 512*16);
    static TASK_INFO_DEF( info_uart2, 512*16);
    static TASK_INFO_DEF( info_led,   512*16);
    static TASK_INFO_DEF( info_led1,   512*16);
    static TASK_INFO_DEF( info_root,  1024*4);

    
    
    arch_interrupt_disable();

    set_irq_stack( (unsigned int)(irq_stack+1024-1) );
    set_abort_stack( (unsigned int)(abort_stack+1024-1) );
    arm_mmu_table_setup((unsigned int)&lds_mmu_table_address );
    enable_mmu( (unsigned int)&lds_mmu_table_address );

    
    rtk_init();
    
    TASK_INIT( "led",  info_led,   6, led_task,  0, 0 );
    TASK_INIT( "led",  info_led1,   5, led_task1,  0, 0 );
    TASK_INIT( "uart", info_uart1, 4, uart_task, "task a p1","task a p2" );
    TASK_INIT( "uart", info_uart2, 4, uart_task, "task b p1","task b p2" );
    TASK_INIT( "root", info_root, 1, main_task, 0,0 );
    TASK_STARTUP(info_led);
    TASK_STARTUP(info_led1);
    TASK_STARTUP(info_uart1);
    TASK_STARTUP(info_uart2);
    TASK_STARTUP(info_root);
    
    rtk_startup();
    
    return 0;
}
