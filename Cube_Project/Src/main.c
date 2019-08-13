/*=============================== INCLUDE ====================================*/
#include "vsf.h"
#include "main.h"
#include <stdio.h>

#define this (*ptThis)
#define TASK_REENTER_RESET_FSM() \
    do {                         \
        this.chState = START;    \
    } while (0)
#define TASK_RESET_FSM()  \
    do {                  \
        s_tState = START; \
    } while (0)

#define INPUT_FIFO_SIZE 30
#define OUTPUT_FIFO_SIZE 100

//! 消息处理函数
typedef struct {
    uint8_t chState;
    print_str_t *ptPrintString;
}cat_handler_pcb_t;

typedef struct {
    uint8_t chState;
    print_str_t *ptPrintString;
}dog_handler_pcb_t;

typedef struct {
    uint8_t chState;
    print_str_t *ptPrintString;
}duck_handler_pcb_t;

static cat_handler_pcb_t s_tCatHandlerPCB; 
static dog_handler_pcb_t s_tDogHandlerPCB;
static duck_handler_pcb_t s_tDuckHandlerPCB;
static fsm_rt_t msg_cat_handler(msg_t *ptMSG);
static fsm_rt_t msg_dog_handler(msg_t *ptMSG);
static fsm_rt_t msg_duck_handler(msg_t *ptMSG);

static uint8_t s_chBytein[INPUT_FIFO_SIZE],s_chByteout[OUTPUT_FIFO_SIZE];
static byte_queue_t s_tFIFOin, s_tFIFOout;

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
     const static msg_t c_tMSGMap[] = {
                        {"cat", &s_tCatHandlerPCB, &msg_cat_handler},
                        {"dog", &s_tCatHandlerPCB, &msg_dog_handler},
                        {"duck", &s_tCatHandlerPCB, &msg_duck_handler}};
     const static search_msg_map_cfg_t c_tSearchMSGMapCFG = {
                                        UBOUND(c_tMSGMap), 
                                        &s_tFIFOin, 
                                        c_tMSGMap};
    static search_msg_map_t s_tSearchMSGMap;
    system_init();
    print_str_pool_item_init();
    INIT_BYTE_QUEUE(&s_tFIFOin, s_chBytein, sizeof(s_chBytein));
    INIT_BYTE_QUEUE(&s_tFIFOout, s_chByteout, sizeof(s_chByteout));
    search_msg_map_init(&s_tSearchMSGMap,&c_tSearchMSGMapCFG);
    LED1_OFF();
    while (1) {
        breath_led();
        msg_map_hanlder(search_msg_map(&s_tSearchMSGMap));
        serial_in_task();
        serial_out_task();
    }
}


static fsm_rt_t msg_cat_handler(msg_t *ptMSG)
{
    cat_handler_pcb_t *ptThis = (cat_handler_pcb_t *)ptMSG->pTarget;
    enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_CAT
    }; 
    switch (this.chState) {
        case START:
            this.chState = INIT;
            //break;
        case INIT:
            this.ptPrintString = print_str_pool_allocate();
            if (this.ptPrintString == NULL) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {
                    "cat has four legs\r\n",
                    &s_tFIFOout,
                    &enqueue_byte
                };
                print_string_init(this.ptPrintString, &c_tCFG);
            } while (0);
            this.chState = PRINT_CAT;
            // break;
        case PRINT_CAT:
            if (fsm_rt_cpl == print_string(this.ptPrintString)) {
                print_str_pool_free(this.ptPrintString);
                TASK_REENTER_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

static fsm_rt_t msg_dog_handler(msg_t *ptMSG)
{
    dog_handler_pcb_t *ptThis = (dog_handler_pcb_t *)ptMSG->pTarget;
    enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_DOG
    };
    switch (this.chState) {
        case START:
            this.chState = INIT;
            //break;
        case INIT:
            this.ptPrintString = print_str_pool_allocate();
            if (this.ptPrintString == NULL) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {
                    "dog has four legs\r\n",
                    &s_tFIFOout,
                    &enqueue_byte
                };
                print_string_init(this.ptPrintString, &c_tCFG);
            } while (0);
            this.chState = PRINT_DOG;
            // break;
        case PRINT_DOG:
            if (fsm_rt_cpl == print_string(this.ptPrintString)) {
                print_str_pool_free(this.ptPrintString);
                TASK_REENTER_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

static fsm_rt_t msg_duck_handler(msg_t *ptMSG)
{
    duck_handler_pcb_t *ptThis = (duck_handler_pcb_t *)ptMSG->pTarget;
    enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_DUCK 
    };
    switch (this.chState) {
        case START:
            this.chState = INIT;
            //break;
        case INIT:
            this.ptPrintString = print_str_pool_allocate();
            if (this.ptPrintString == NULL) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {
                    "duck has two legs\r\n",
                    &s_tFIFOout,
                    &enqueue_byte
                };
                print_string_init(this.ptPrintString, &c_tCFG);
            } while (0);
            this.chState = PRINT_DUCK;
            // break;
        case PRINT_DUCK:
            if (fsm_rt_cpl == print_string(this.ptPrintString)) {
                print_str_pool_free(this.ptPrintString);
                TASK_REENTER_RESET_FSM();
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
