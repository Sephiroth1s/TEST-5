/*=============================== INCLUDE ====================================*/
#include "vsf.h"
#include "main.h"
#include <stdio.h>

#define this (*ptThis)
#define TASK_REENTER_RESET_FSM() \
    do {                         \
        this.chState = START;    \
    } while (0);
#define TASK_RESET_FSM()  \
    do {                  \
        s_tState = START; \
    } while (0);
#define TASK_CHECK_RESET_FSM() \
    do {                       \
        this.chState = START;  \
    } while (0);

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

typedef struct {
    uint8_t chState;
    check_str_t tCheckHello;
} check_hello_pcb_t;

typedef struct {
    uint8_t chState;
    check_str_t tCheckOrange;
} check_orange_pcb_t;

typedef struct {
    uint8_t chState;
    check_str_t tCheckApple;
} check_apple_pcb_t;

static cat_handler_pcb_t s_tCatHandlerPCB;
static dog_handler_pcb_t s_tDogHandlerPCB;
static duck_handler_pcb_t s_tDuckHandlerPCB;
static void msg_cat_handler(msg_t *ptMSG);
static void msg_dog_handler(msg_t *ptMSG);
static void msg_duck_handler(msg_t *ptMSG);

static uint8_t s_chBytein[INPUT_FIFO_SIZE], s_chByteout[OUTPUT_FIFO_SIZE];
static byte_queue_t s_tFIFOin, s_tFIFOout;

static event_t s_tPrintWorld, s_tPrintApple, s_tPrintOrange;

static check_hello_pcb_t s_tCheckHelloPCB;
static check_apple_pcb_t s_tCheckApplePCB;
static check_orange_pcb_t s_tCheckOrangePCB;

static fsm_rt_t task_print_world(void);
static fsm_rt_t task_print_apple(void);
static fsm_rt_t task_print_orange(void);

static fsm_rt_t task_world(void);
static fsm_rt_t task_apple(void);
static fsm_rt_t task_orange(void);

static fsm_rt_t check_hello(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop);
static fsm_rt_t check_apple(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop);
static fsm_rt_t check_orange(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop);

static uint8_t s_chPrintStrPool[120] ALIGN(__alignof__(print_str_t));
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
    const static check_msg_map_cfg_t c_tCheckMSGMapCFG = {
                                        UBOUND(c_tMSGMap), 
                                        &s_tFIFOin, 
                                        c_tMSGMap};
    static check_msg_map_t s_tCheckMSGMap;

    static const check_agent_t c_tCheckWordsAgent[] = {
                                {&s_tCheckHelloPCB, check_hello},
                                {&s_tCheckApplePCB, check_apple},
                                {&s_tCheckOrangePCB, check_orange},
                                {&s_tCheckMSGMap, check_msg_map}};
    static const check_use_peek_cfg_t c_tCheckWordsUsePeekCFG = {
                                        UBOUND(c_tCheckWordsAgent),
                                        &s_tFIFOin,
                                        (check_agent_t *)c_tCheckWordsAgent};
    static check_use_peek_t s_tCheckWordsUsePeek;

    system_init();
    print_str_pool_item_init();
    INIT_EVENT(&s_tPrintWorld, false, false);
    INIT_EVENT(&s_tPrintApple, false, false);
    INIT_EVENT(&s_tPrintOrange, false, false);
    print_str_pool_add_heap(s_chPrintStrPool,UBOUND(s_chPrintStrPool));
    INIT_BYTE_QUEUE(&s_tFIFOin, s_chBytein, sizeof(s_chBytein));
    INIT_BYTE_QUEUE(&s_tFIFOout, s_chByteout, sizeof(s_chByteout));
    check_msg_map_init(&s_tCheckMSGMap, &c_tCheckMSGMapCFG);
    check_use_peek_init(&s_tCheckWordsUsePeek, &c_tCheckWordsUsePeekCFG);
    LED1_OFF();
    while (1) {
        breath_led();
        task_print_world();
        task_print_apple();
        task_print_orange();
        task_check_use_peek(&s_tCheckWordsUsePeek);
        serial_in_task();
        serial_out_task();
    }
}

static void msg_cat_handler(msg_t *ptMSG)
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
        GOTO_PRINT_CAT:
            if (fsm_rt_cpl == print_string(this.ptPrintString)) {
                print_str_pool_free(this.ptPrintString);
                TASK_REENTER_RESET_FSM();
            }else{
                goto GOTO_PRINT_CAT;
            }
            break;
        default:
            break;
    }
}

static void msg_dog_handler(msg_t *ptMSG)
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
        GOTO_PRINT_DOG:
            if (fsm_rt_cpl == print_string(this.ptPrintString)) {
                print_str_pool_free(this.ptPrintString);
                TASK_REENTER_RESET_FSM();
            } else {
                goto GOTO_PRINT_DOG;
            }
            break;
        default:
            break;
    }
}

static void msg_duck_handler(msg_t *ptMSG)
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
        GOTO_PRINT_DUCK:
            if (fsm_rt_cpl == print_string(this.ptPrintString)) {
                print_str_pool_free(this.ptPrintString);
                TASK_REENTER_RESET_FSM();
            } else {
                goto GOTO_PRINT_DUCK;
            }
            break;
        default:
            break;
    }
}

static fsm_rt_t task_print_world(void)
{
    static enum {
        START,
        PRINT_WORLD
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = START;
            // break;
        case PRINT_WORLD:
            if (fsm_rt_cpl == task_world()) {
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

static fsm_rt_t task_world(void)
{
    static print_str_t *s_ptPrintString;
    static enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_WORLD
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = WAIT_PRINT;
            //break;
        case WAIT_PRINT:
            if (WAIT_EVENT(&s_tPrintWorld)) {
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
                    "world\r\n",
                    &s_tFIFOout,
                    &enqueue_byte
                };
                print_string_init(s_ptPrintString, &c_tCFG);
            } while (0);
            s_tState = PRINT_WORLD;
            // break;
        case PRINT_WORLD:
            if (fsm_rt_cpl == print_string(s_ptPrintString)) {
                print_str_pool_free(s_ptPrintString);
                RESET_EVENT(&s_tPrintWorld);
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

static fsm_rt_t task_print_apple(void)
{
    static enum {
        START,
        PRINT_APPLE
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = START;
            // break;
        case PRINT_APPLE:
            if (fsm_rt_cpl == task_apple()) {
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

static fsm_rt_t task_apple(void)
{
    static print_str_t *s_ptPrintString;
    static enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_APPLE
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = WAIT_PRINT;
            //break;
        case WAIT_PRINT:
            if (WAIT_EVENT(&s_tPrintApple)) {
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
                    "apple\r\n",
                    &s_tFIFOout,
                    &enqueue_byte
                };
                print_string_init(s_ptPrintString, &c_tCFG);
            } while (0);
            s_tState = PRINT_APPLE;
            // break;
        case PRINT_APPLE:
            if (fsm_rt_cpl == print_string(s_ptPrintString)) {
                print_str_pool_free(s_ptPrintString);
                RESET_EVENT(&s_tPrintApple);
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

static fsm_rt_t task_print_orange(void)
{
    static enum {
        START,
        PRINT_ORANGE
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = START;
            // break;
        case PRINT_ORANGE:
            if (fsm_rt_cpl == task_orange()) {
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

static fsm_rt_t task_orange(void)
{
    static print_str_t *s_ptPrintString;
    static enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_ORANGE
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = WAIT_PRINT;
            //break;
        case WAIT_PRINT:
            if (WAIT_EVENT(&s_tPrintOrange)) {
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
                    "orange\r\n",
                    &s_tFIFOout,
                    &enqueue_byte
                };
                print_string_init(s_ptPrintString, &c_tCFG);
            } while (0);
            s_tState = PRINT_ORANGE;
            // break;
        case PRINT_ORANGE:
            if (fsm_rt_cpl == print_string(s_ptPrintString)) {
                print_str_pool_free(s_ptPrintString);
                RESET_EVENT(&s_tPrintOrange);
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
                SET_EVENT(&s_tPrintWorld);
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
                SET_EVENT(&s_tPrintApple);
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
                SET_EVENT(&s_tPrintOrange);
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
