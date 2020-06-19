#define SW_MUX_GPIO1_IO03_BASE *((volatile unsigned int *)0x02030068)
#define SW_PAD_GPIO1_IO03_BASE *((volatile unsigned int *)0x020e02f4)
#define GPIO1_GDIR *((volatile unsigned int *)0x0209c004)
#define GPIO1_DR *((volatile unsigned int *)0x0209c000)

void led_init(){
    SW_MUX_GPIO1_IO03_BASE = 0x05;
    SW_PAD_GPIO1_IO03_BASE = 0x10B0;
    GPIO1_GDIR = 0x08;
}

void led_on(){
    GPIO1_DR &= ~(1<<3);
}

void led_off(){
    GPIO1_DR |= (1<<3);
}

void delay_ms(unsigned int msecs){
    unsigned int i;
    while(msecs--){
        for(i = 0x7ff; i > 0; i--){
            ;
        }
    }
}


int main(void){
    led_init();
    while(1){
        led_on();
        delay_ms(500);
        led_off();
        delay_ms(500);
    }
    return 0;
}