/*=============================== INCLUDE ====================================*/
#include "vsf.h"
#include "main.h"
#include <stdio.h>

#define RESET_ALL_ATTRIBUTES "\033[0m"		// 关闭所有属性
#define SET_BRIGHT "\033[1m"		// 设置为高亮
#define SET_UNDERSCORE "\033[4m"		// 下划线
#define SET_BLINK "\033[5m"		// 闪烁
#define SET_REVERSE "\033[7m"		// 反显
#define SET_HIDDEN "\033[8m"		// 消隐
#define CURSOR_UP "\033[A"		// 光标上移 1 行
#define CURSOR_DOWN "\033[B"		// 光标下移 1 行
#define CURSOR_RIGHT "\033[C"		// 光标右移 1 行
#define CURSOR_LEFT "\033[D"		// 光标左移 1 行
#define ENTER   "\x0A\x0D"
#define CLEAR_SCREEN "\033[2J"		// 清屏
#define ERASE_END_OF_LINE "\033[K"		// 清除从光标到行尾的内容
#define ERASE_LINE "\033[2K"        //  清楚当前行
#define SAVE_CURSOR "\033[s"		// 保存光标位置
#define UNSAVE_CURSOR "\033[u"		// 恢复光标位置
#define HIDDEN_CURSOR "\033[?25l"	// 隐藏光标
#define DISPLAY_CURSOR "\033[?25h"	// 显示光标
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

static POOL(print_str) s_tPrintFreeList;
static POOL(check_str) s_tCheckFreeList;


static void msg_cat_handler(msg_t *ptMSG);
static void msg_dog_handler(msg_t *ptMSG);
static void msg_duck_handler(msg_t *ptMSG);

static uint8_t s_chBytein[INPUT_FIFO_SIZE], s_chByteout[OUTPUT_FIFO_SIZE];
static byte_queue_t s_tFIFOin, s_tFIFOout;

static event_t s_tPrintWorld, s_tPrintApple, s_tPrintOrange, s_tCatHandlerEvent, s_tDogHandlerEvent, s_tDuckHandlerEvent;
static event_t s_tHandlerEvent,s_tHandlerEventEnd;
static msg_t s_tCurrentMsg;

static uint8_t s_chPrintStrPool[256] ALIGN(__alignof__(print_str_t));
static uint8_t s_chCheckStrPool[256] ALIGN(__alignof__(check_str_t));

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

    POOL_INIT(print_str, &s_tPrintFreeList);
    POOL_ADD_HEAP(print_str, &s_tPrintFreeList, s_chPrintStrPool, UBOUND(s_chPrintStrPool));
    INIT_BYTE_QUEUE(&s_tFIFOin, s_chBytein, sizeof(s_chBytein));
    INIT_BYTE_QUEUE(&s_tFIFOout, s_chByteout, sizeof(s_chByteout));
    LED1_OFF();
    while (1) {
        breath_led();

        serial_in_task();
        serial_out_task();
    }
}

void msg_handler(msg_t*ptMSG)
{
    if ((ptMSG != NULL) && (ptMSG->pTarget != NULL)) {
        SET_EVENT(ptMSG->pTarget);
        if (WAIT_EVENT(&s_tHandlerEventEnd)) {
            s_tCurrentMsg = *ptMSG;
        }
    }
}

fsm_rt_t task_msg_handler(void) 
{
    static print_str_t *s_ptPrintString;
    static enum {
        START,
        INIT,
        WAIT_PRINT,
        PRINT_WORDS
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = WAIT_PRINT;
            //break;
        case WAIT_PRINT:
            if (WAIT_EVENT(&s_tHandlerEvent)) {
                s_tState = INIT;
                //break;
            } else {
                break;
            }
        case INIT:
            s_ptPrintString = POOL_ALLOCATE(print_str,&s_tPrintFreeList);
            if (s_ptPrintString == NULL) {
                break;
            }
            do {
                static uint8_t s_chBuffer[30];
                sprintf(s_chBuffer,"%.5s Key is pressed.\r\n",true_key(s_tCurrentMsg.pchMessage));
                const print_str_cfg_t c_tCFG = {
                    s_chBuffer,
                    &s_tFIFOout,
                    &enqueue_byte
                };
                PRINT_STRING.Init(s_ptPrintString, &c_tCFG);
            } while (0);
            s_tState = PRINT_WORDS;
            // break;
        case PRINT_WORDS:
            if (fsm_rt_cpl == PRINT_STRING.Print(s_ptPrintString)) {
                POOL_FREE(print_str, &s_tPrintFreeList, s_ptPrintString);
                SET_EVENT(&s_tHandlerEventEnd);
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

static uint8_t* true_key(uint8_t* pchKey)
{
    uint8_t *chKey = pchKey + 2;
    switch (*chKey) {
        case 0x50:
            return "F1";
            break;
        case 0x51:
            return "F2";
            break;
        case 0x52:
            return "F3";
            break;
        case 0x53:
            return "F4";
            break;
        case 0x54:
            return "F5";
            break;
        case 0x55:
            return "F6";
            break;
        case 0x56:
            return "F7";
            break;
        case 0x57:
            return "F8";
            break;
        case 0x58:
            return "F9";
            break;
        case 0x59:
            return "F10";
            break;
        case 0x5A:
            return "F11";
            break;
        case 0x5B:
            return "F12";
            break;
        case 0x41:
            return "UP";
            break;
        case 0x42:
            return "DOWN";
            break;
        case 0x43:
            return "RIGHT";
            break;
        case 0x44:
            return "LEFT";
            break;
        default:
            return  pchKey;
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
            s_ptPrintString = POOL_ALLOCATE(print_str,&s_tPrintFreeList);
            if (s_ptPrintString == NULL) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {
                    "world\r\n",
                    &s_tFIFOout,
                    &enqueue_byte
                };
                PRINT_STRING.Init(s_ptPrintString, &c_tCFG);
            } while (0);
            s_tState = PRINT_WORLD;
            // break;
        case PRINT_WORLD:
            if (fsm_rt_cpl == PRINT_STRING.Print(s_ptPrintString)) {
                POOL_FREE(print_str,&s_tPrintFreeList,s_ptPrintString);
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

bool print_str_output_byte(void *ptThis, uint8_t pchByte)
{
    if (ENQUEUE_BYTE(ptThis, pchByte)) {
        return true;
    }
    return false;
}
typedef struct 
{
    uint8_t chState;
    byte_queue_t * ptFIFOin;
    print_str_t * ptPrintStr;
    uint8_t* pchBuffer;
    uint8_t chCounter;
    uint8_t chMaxNumber;
}console_print_t;
