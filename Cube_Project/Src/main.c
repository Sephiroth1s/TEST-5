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
#define INIT_CURSOR "\033[2K>\033[s"		// 清楚当前+保存光标位置+'>'符号
#define UNSAVE_CURSOR "\033[u"		// 恢复光标位置
#define HIDDEN_CURSOR "\033[?25l"	// 隐藏光标
#define DISPLAY_CURSOR "\033[?25h"	// 显示光标
#define this (*ptThis)
#define TASK_CONSOLE_RESET_FSM() \
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

typedef struct
{
    uint8_t chState;
    print_str_t *ptPrintStr;
}update_line_t;

typedef struct {
    uint8_t chState;
    read_byte_evt_handler_t *ptReadByteEvent;
    print_str_t *ptPrintStr;
    uint8_t *pchBuffer;
    uint8_t chByte;
    uint8_t chCounter;
    uint8_t chMaxNumber;
    update_line_t ptTarget;
} console_print_t;

typedef struct {
    read_byte_evt_handler_t *ptReadByteEvent;
    uint8_t chMaxNumber;
    uint8_t *pchBuffer;
} console_print_cfg_t;

static POOL(print_str) s_tPrintFreeList;

static uint8_t s_chBytein[INPUT_FIFO_SIZE], s_chByteout[OUTPUT_FIFO_SIZE];
static byte_queue_t s_tFIFOin, s_tFIFOout;

static uint8_t s_chPrintStrPool[256] ALIGN(__alignof__(print_str_t));

static bool task_console_init(console_print_t *ptThis,console_print_cfg_t *ptCFG);
static fsm_rt_t task_console(console_print_t *ptThis);
static fsm_rt_t update_line(update_line_t *ptThis);

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
    const static console_print_cfg_t c_tCFG;
    static console_print_t s_tConsole
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

bool task_console_init(console_print_t *ptThis,console_print_cfg_t *ptCFG)
{
    enum { 
        START 
    };
    if (       (NULL == ptThis) 
            || (NULL == ptCFG) 
            || (NULL == this.ptTarget) 
            || (NULL == ptCFG->pchBuffer) 
            || (NULL == ptCFG->ptReadByteEvent) 
            || (NULL == ptCFG->ptReadByteEvent->fnReadByte)) {
        return false;
    }
    this.chState = START;
    this.chMaxNumber = ptCFG->chMaxNumber;
    this.pchBuffer = ptCFG->pchBuffer;
    this.ptReadByteEvent = this.ptReadByteEvent;
    this.ptTarget.chState = START;
    return true;
}

fsm_rt_t task_console(console_print_t *ptThis)
{
    enum {
        START,
        RESUME_CURSOR,
        PRINT_RESUME,
        READ_BYTE,
        CHECK_BYTE,
        CHECK_ENTER,
        CHECK_DELETE,
        WRITE_BUFFER,
        UPDATE_LINE,
        DELETE_BYTE,
        INIT_PRINT,
        PRINT_BUFFER
    };
    uint8_t *pchTemp;
    if (NULL == ptThis) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chState = RESUME_CURSOR;
            // break;
        case RESUME_CURSOR:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        INIT_CURSOR, 
                        &s_tFIFOout,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_RESUME;
                // break;
            }
        case PRINT_RESUME:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                this.chState = READ_BYTE;
                // break;
            } else {
                break;
            }
        case READ_BYTE:
            if (this.ptReadByteEvent->fnReadByte(this.ptReadByteEvent->pTarget,
                                                 &this.chByte)) {
                this.chState = CHECK_BYTE;
                // break;
            } else {
                break;
            }
        case CHECK_BYTE:
            if (this.chByte > 31 && this.chByte < 127) {
                this.chState = WRITE_BUFFER;
                // break;
            } else {
                this.chState = CHECK_ENTER;
                goto GOTO_CHECK_ENTER;
            }
        case WRITE_BUFFER:
            pchTemp = this.pchBuffer + this.chCounter;
            if (this.chCounter < this.chMaxNumber) {
                *pchTemp++ = this.chByte;
                pchTemp = '\0';
                this.chCounter++;
            }
            this.chState = INIT_PRINT;
            // break;
        case INIT_PRINT:
        GOTO_INIT_PRINT:
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        this.pchBuffer, 
                        &s_tFIFOout,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_BUFFER;
                // break;
            }
        case PRINT_BUFFER:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        case CHECK_ENTER:
        GOTO_CHECK_ENTER:
            if (this.chByte == '\x0d') {
                this.chState = UPDATE_LINE;
            } else {
                this.chState = CHECK_DELETE;
                goto GOTO_CHECK_DELETE;
            }
            // break;
        case UPDATE_LINE:
            if (fsm_rt_cpl == update_line(this.ptTarget)) {
                this.chState = INIT_PRINT;
                goto GOTO_INIT_PRINT;
            }
            break;
        case CHECK_DELETE:
        GOTO_CHECK_DELETE:
            if (this.chByte == '\x7F') {
                this.chState = DELETE_BYTE;
                // break;
            } else {
                TASK_CONSOLE_RESET_FSM();
                break;
            }
        case DELETE_BYTE:
            if (this.chCounter > 0) {
                this.chCounter--;
                pchTemp = this.pchBuffer + this.chCounter;
                *pchTemp = '\0';
            } else {
                this.chCounter = 0;
                pchTemp = this.pchBuffer + this.chCounter;
                *pchTemp = '\0';
            }
            this.chState = INIT_PRINT;
            goto GOTO_INIT_PRINT;
        default:
            return fsm_rt_err;
            break;
    }
}

fsm_rt_t update_line(update_line_t *ptThis)
{
    enum {
        START,
        RESUME_CURSOR,
        PRINT_RESUME,
        CLEAR_TO_END,
        PRINT_CLEAR,
        INIT_ENTER,
        PRINT_ENTER,
        SAVE_NEW_CURSOR,
        PRINT_SAVE
    };
    switch (this.chState) {
        case START:
            this.chState = RESUME_CURSOR;
            // break;
        case RESUME_CURSOR:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        INIT_CURSOR, 
                        &s_tFIFOout,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_RESUME;
                // break;
            }
        case PRINT_RESUME:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                this.chState = CLEAR_TO_END;
                // break;
            } else {
                break;
            }
        case CLEAR_TO_END:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        INIT_CURSOR, 
                        &s_tFIFOout,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_CLEAR;
                // break;
            }
        case PRINT_CLEAR:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                this.chState = INIT_ENTER;
                // break;
            } else {
                break;
            }
        case INIT_ENTER:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        INIT_CURSOR, 
                        &s_tFIFOout,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_ENTER;
                // break;
            }
        case PRINT_ENTER:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                this.chState = SAVE_NEW_CURSOR;
                // break;
            } else {
                break;
            }
        case SAVE_NEW_CURSOR:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        INIT_CURSOR, 
                        &s_tFIFOout,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_SAVE;
                // break;
            }
        case PRINT_SAVE:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                this.chState = PRINT_SAVE;
                // break;
            } else {
                break;
            }
        default:
            break;
    }
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
