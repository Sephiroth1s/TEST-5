/*=============================== INCLUDE ====================================*/
#include "vsf.h"
#include "main.h"
#include <stdio.h>

extern bool serial_out(uint8_t chByte);
extern bool serial_in(uint8_t *pchByte);

static void system_init(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    system_clock_config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    vsf_stdio_init();

}

/*================================= MAIN =====================================*/
int main(void)
{
    system_init();
    
    uint8_t chByte;
    while (1) {
        if(serial_in(&chByte)) {
            serial_out(chByte);
            printf("0x%02x ", chByte);
        }
    }
}


