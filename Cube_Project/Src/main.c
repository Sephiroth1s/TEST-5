/*=============================== INCLUDE ====================================*/
#include "vsf.h"
#include "main.h"
#include <stdio.h>
#include "main.h"
#include "../User_App/queue/queue.h"
#include "../User_App/led/led.h"

#define this (*ptThis)
#define TASK_CHECK_RESET_FSM()      \
    do {                      \
        this.chState = START; \
    } while (0)
#define TASK_RESET_FSM()  \
    do {                  \
        s_tState = START; \
    } while (0)

#define INPUT_FIFO_SIZE 30
#define OUTPUT_FIFO_SIZE 100

#define FN_ENQUEUE_BYTE (&enqueue_byte)
#define FN_DEQUEUE_BYTE (&dequeue_byte)
#define FN_PEEK_BYTE_QUEUE (&peek_byte_queue)


//! 消息处理函数
static fsm_rt_t msg_cat_handler(msg_t *ptMSG);
static fsm_rt_t msg_dog_handler(msg_t *ptMSG);
static fsm_rt_t msg_duck_handler(msg_t *ptMSG);
 
//! 消息地图
const static msg_t c_tMSGMap[] = {
    {“cat”, &msg_cat_handler},
    {“dog”, &msg_dog_handler},
    {“duck”, &msg_duck_handler},
};

static event_t s_tPrintCat, s_tPrintDog, s_tPrintDuck;
static uint8_t s_chBytein[INPUT_FIFO_SIZE],s_chByteout[OUTPUT_FIFO_SIZE];
static byte_queue_t s_tFIFOin, s_tFIFOout;

static fsm_rt_t task_print_cat(void);
static fsm_rt_t task_print_dog(void);
static fsm_rt_t task_print_duck(void);

static fsm_rt_t print_cat(void);
static fsm_rt_t print_dog(void);
static fsm_rt_t print_duck(void);

static fsm_rt_t check_hello(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop);
static fsm_rt_t check_apple(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop);
static fsm_rt_t check_orange(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop);

msg_t *search_msg_map(msg_t *ptMSG)

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
    print_str_pool_item_init();
    INIT_EVENT(&s_tPrintCat, false, false);
    INIT_EVENT(&s_tPrintDog, false, false);
    INIT_EVENT(&s_tPrintDuck, false, false);
    INIT_BYTE_QUEUE(&s_tFIFOin, s_chBytein, sizeof(s_chBytein));
    INIT_BYTE_QUEUE(&s_tFIFOout, s_chByteout, sizeof(s_chByteout));
    LED1_OFF();
    while (1) {
        breath_led();
        task_print_cat();
        task_print_dog();
        task_print_duck();
        serial_in_task();
        serial_out_task();
    }
}

static fsm_rt_t task_print_cat(void)
{
    static enum {
        START,
        PRINT_CAT
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = START;
            // break;
        case PRINT_CAT:
            if (fsm_rt_cpl == print_cat()) {
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

static fsm_rt_t print_cat(void)
{
    static print_str_t *s_ptPrintString;
    static enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_CAT
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = WAIT_PRINT;
            //break;
        case WAIT_PRINT:
            if (WAIT_EVENT(&s_tPrintCat)) {
                s_tState = INIT;
                //break;
            } else {
                break;
            }
        case INIT:
            s_ptPrintString = print_str_pool_allocate();
            if (s_ptPrintString == NULL) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {
                    "cat has four legs\r\n",
                    &s_tFIFOout,
                    FN_ENQUEUE_BYTE
                };
                print_string_init(s_ptPrintString, &c_tCFG);
            } while (0);
            s_tState = PRINT_CAT;
            // break;
        case PRINT_CAT:
            if (fsm_rt_cpl == print_string(s_ptPrintString)) {
                print_str_pool_free(s_ptPrintString);
                RESET_EVENT(&s_tPrintCat);
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

static fsm_rt_t task_print_dog(void)
{
    static enum {
        START,
        PRINT_DOG
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = START;
            // break;
        case PRINT_DOG:
            if (fsm_rt_cpl == print_dog()) {
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

static fsm_rt_t print_dog(void)
{
    static print_str_t *s_ptPrintString;
    static enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_DOG
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = WAIT_PRINT;
            //break;
        case WAIT_PRINT:
            if (WAIT_EVENT(&s_tPrintDog)) {
                s_tState = INIT;
                // break;
            } else {
                break;
            }
        case INIT:
            s_ptPrintString = print_str_pool_allocate();
            if (s_ptPrintString == NULL) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {
                    "dog has four legs\r\n",
                    &s_tFIFOout,
                    FN_ENQUEUE_BYTE
                };
                print_string_init(s_ptPrintString, &c_tCFG);
            } while (0);
            s_tState = PRINT_DOG;
            // break;
        case PRINT_DOG:
            if (fsm_rt_cpl == print_string(s_ptPrintString)) {
                print_str_pool_free(s_ptPrintString);
                RESET_EVENT(&s_tPrintDog);
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

static fsm_rt_t task_print_duck(void)
{
    static enum {
        START,
        PRINT_DUCK
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = START;
            // break;
        case PRINT_DUCK:
            if (fsm_rt_cpl == print_duck()) {
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

static fsm_rt_t print_duck(void)
{
    static print_str_t *s_ptPrintString;
    static enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_DUCK
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = WAIT_PRINT;
            //break;
        case WAIT_PRINT:
            if (WAIT_EVENT(&s_tPrintDuck)) {
                s_tState = INIT;
                // break;
            } else {
                break;
            }
        case INIT:
            s_ptPrintString = print_str_pool_allocate();
            if (s_ptPrintString == NULL) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {
                    "duck has two legs\r\n",
                    &s_tFIFOout,
                    FN_ENQUEUE_BYTE
                };
                print_string_init(s_ptPrintString, &c_tCFG);
            } while (0);
            s_tState = PRINT_DUCK;
            // break;
        case PRINT_DUCK:
            if (fsm_rt_cpl == print_string(s_ptPrintString)) {
                print_str_pool_free(s_ptPrintString);
                RESET_EVENT(&s_tPrintDuck);
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


fsm_rt_t check_hello(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop)
{
    check_hello_pcb_t *ptThis = (check_hello_pcb_t *)pTarget;
    enum {
        START,
        CHECK_STRING
    };
    switch (this.chState) {
        case START:
            do {
                const check_str_cfg_t c_tCFG = {
                    "hello",
                    ptReadByte
                };
                check_string_init(&this.tCheckHello, &c_tCFG);
            } while (0);
            this.chState = CHECK_STRING;
            // break;
        case CHECK_STRING:
            *pbRequestDrop = false;
            if (fsm_rt_cpl == check_string(&this.tCheckHello, pbRequestDrop)) {
                SET_EVENT(&s_tPrintCat);
                TASK_CHECK_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

static fsm_rt_t check_apple(void *pTarget, read_byte_evt_handler_t *ptReadByte,  bool *pbRequestDrop)
{
    check_apple_pcb_t *ptThis=(check_apple_pcb_t *)pTarget;
    enum {
        START,
        CHECK_STRING
    };
    switch (this.chState) {
        case START:
            do {
                const check_str_cfg_t c_tCFG = {
                    "apple",
                    ptReadByte
                };
                check_string_init(&this.tCheckApple, &c_tCFG);
            } while (0);
            this.chState = CHECK_STRING;
            // break;
        case CHECK_STRING:
            *pbRequestDrop = false;
            if (fsm_rt_cpl == check_string(&this.tCheckApple, pbRequestDrop)) {
                SET_EVENT(&s_tPrintDog);
                TASK_CHECK_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

static fsm_rt_t check_orange(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop)
{
    check_orange_pcb_t *ptThis = (check_orange_pcb_t *)pTarget;
    enum {
        START,
        CHECK_STRING
    };
    switch (this.chState) {
        case START:
            do {
                const check_str_cfg_t c_tCFG = {
                    "orange",
                    ptReadByte
                };
                check_string_init(&this.tCheckOrange, &c_tCFG);
            } while (0);
            this.chState = CHECK_STRING;
            // break;
        case CHECK_STRING:
            *pbRequestDrop = false;
            if (fsm_rt_cpl == check_string(&this.tCheckOrange, pbRequestDrop)) {
                SET_EVENT(&s_tPrintDuck);
                TASK_CHECK_RESET_FSM();
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
