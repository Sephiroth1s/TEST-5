/*=============================== INCLUDE ====================================*/
#include "vsf.h"
#include "main.h"
#include <stdio.h>

#include "plooc.h"
#define __MAIN_CLASS_IMPLEMENT
#define __PLOOC_CLASS_USE_STRICT_TEMPLATE__
#if     defined(__MAIN_CLASS_IMPLEMENT)
#       define __PLOOC_CLASS_IMPLEMENT
#elif   defined(__MAIN_CLASS_INHERIT)
#       define __PLOOC_CLASS_INHERIT
#endif  
#include "plooc_class.h"

#define SERIAL_TASK_RESET_FSM() do { s_tState = START; } while(0)
#define TASK_RESET_FSM() do { this.chState = START; } while(0)
#ifndef this
    #define this (*ptThis)
#endif
#define ENTER "\x0A\x0D"
#define INPUT_FIFO_SIZE 30
#define OUTPUT_FIFO_SIZE 100

declare_class(print_key_t)
def_class(print_key_t,
    private_member (
        uint8_t chState;
        print_str_t *ptPrintStr;
        key_t tKeyEvent;
        key_service_t *ptTarget;
    )
)
end_def_class(print_key_t)
    
extern POOL(print_str) s_tPrintFreeList;

static uint8_t s_chBytein[INPUT_FIFO_SIZE], s_chByteout[OUTPUT_FIFO_SIZE];
static byte_queue_t s_tFIFOin, s_tFIFOout;

static uint8_t s_chPrintStrPool[256] ALIGN(__alignof__(print_str_t));
static fsm_rt_t print_key(print_key_t *ptObj);
extern bool serial_out(uint8_t chByte);
extern bool serial_in(uint8_t *pchByte);
bool print_str_output_byte(void *ptThis, uint8_t pchByte);
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
    enum { START };
    static key_service_t s_tKeyService;
    static print_key_t s_tPrintKey;
    do {
        class_internal(&s_tPrintKey, ptThis, print_key_t);
        this.ptTarget = &s_tKeyService;
    } while(0);
    system_init();
    led_init();
    POOL_INIT(print_str, &s_tPrintFreeList);
    POOL_ADD_HEAP(print_str, &s_tPrintFreeList, s_chPrintStrPool, UBOUND(s_chPrintStrPool));
    INIT_BYTE_QUEUE(&s_tFIFOin, s_chBytein, sizeof(s_chBytein));
    INIT_BYTE_QUEUE(&s_tFIFOout, s_chByteout, sizeof(s_chByteout));
    KEY_SERVICE.Init(&s_tKeyService);
    LED1_OFF();
    key_init();
    while (1) {
        breath_led();
        KEY_SERVICE.Task(&s_tKeyService);
        print_key(&s_tPrintKey);
        serial_in_task();
        serial_out_task();
    }
}
fsm_rt_t print_key(print_key_t *ptObj)
{
    class_internal(ptObj, ptThis, print_key_t);
    enum {
        START,
        GET_KEY,
        INIT_PRINT_KEY_UP,
        PRINT_KEY_UP,
        INIT_PRINT_KEY_DOWN,
        PRINT_KEY_DOWN,
    };
    if (NULL == ptObj) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chState = GET_KEY;
            // break;
        case GET_KEY:
            if(!KEY_SERVICE.GetKey(this.ptTarget, &this.tKeyEvent)){
                break;
            }
            if (KEY_UP == this.tKeyEvent.tEvent) {
                this.chState = INIT_PRINT_KEY_UP;
                goto GOTO_INIT_PRINT_KEY_UP;
            }
            if (KEY_DOWN == this.tKeyEvent.tEvent) {
                this.chState = INIT_PRINT_KEY_DOWN;
                goto GOTO_INIT_PRINT_KEY_DOWN;
            }
            break;
        case INIT_PRINT_KEY_UP:
        GOTO_INIT_PRINT_KEY_UP:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (NULL == this.ptPrintStr) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {"KEY1 UP\r\n", &s_tFIFOout};
                PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
            } while (0);
            this.chState = PRINT_KEY_UP;
            // break;
        case PRINT_KEY_UP:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                TASK_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        case INIT_PRINT_KEY_DOWN:
        GOTO_INIT_PRINT_KEY_DOWN:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (NULL == this.ptPrintStr) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {"KEY1 DOWN\r\n", &s_tFIFOout};
                PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
            } while (0);
            this.chState = PRINT_KEY_DOWN;
            // break;
        case PRINT_KEY_DOWN:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                TASK_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}
fsm_rt_t serial_in_task(void)
{
    static enum {
        START,
        REDA_AND_ENQUEUE
    } s_tState = START;
    uint8_t chByte;
    switch (s_tState) {
        case START:
            s_tState = START;
            //break;
        case REDA_AND_ENQUEUE:
            if (serial_in(&chByte)) {
                ENQUEUE_BYTE(&s_tFIFOin, chByte);
                SERIAL_TASK_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

fsm_rt_t serial_out_task(void)
{
    static enum {
        START,
        DEQUEUE,
        SEND_BYTE
    } s_tState = START;
    static uint8_t s_chByte;
    switch (s_tState) {
        case START:
            s_tState = START;
            //break;
        case DEQUEUE:
            if (!DEQUEUE_BYTE(&s_tFIFOout, &s_chByte)) {
                break;
            } else {
                s_tState = SEND_BYTE;
            }
            //break;
        case SEND_BYTE:
            if (serial_out(s_chByte)) {
                SERIAL_TASK_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

bool print_str_output_byte(void *ptThis, uint8_t pchByte)
{
    if (ENQUEUE_BYTE(&s_tFIFOout, pchByte)) {
        return true;
    }
    return false;
}
