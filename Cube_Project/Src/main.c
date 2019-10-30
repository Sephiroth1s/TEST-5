/*=============================== INCLUDE ====================================*/
#include "vsf.h"
#include "main.h"
#include <stdio.h>

#define TASK_RESET_FSM()  \
    do {                  \
        s_tState = START; \
    } while (0);
#define TASK_CHECK_RESET_FSM() \
    do {                       \
        this.chState = START;  \
    } while (0);
#define TASK_CONSOLE_RESET_FSM() \
    do {                       \
        this.chState = START;  \
    } while (0);

#define ENTER "\x0A\x0D"
#define INPUT_FIFO_SIZE 30
#define OUTPUT_FIFO_SIZE 100
#define CONSOLE_BUFFER_SIZE 50
#define CONSOLE_INPUT_SIZE 50

typedef struct print_token_t print_token_t;
struct print_token_t{
    uint8_t chState;
    void *pTarget;
    uint8_t *pchTokensArray;
    uint16_t hwTokens;
    print_str_t *ptPrintStr;
};

extern POOL(print_str) s_tPrintFreeList;

static uint8_t s_chBytein[INPUT_FIFO_SIZE], s_chByteout[OUTPUT_FIFO_SIZE], s_chByteConsole[CONSOLE_INPUT_SIZE];
static byte_queue_t s_tFIFOin, s_tFIFOout, s_tFIFOConsolein;
static event_t s_tRepeatLineEvent,s_tRepeatByteEvent;

static uint8_t s_chPrintStrPool[256] ALIGN(__alignof__(print_str_t));

static bool console_input(uint8_t chByte);
static fsm_rt_t print_token(print_token_t *ptThis, uint8_t *pchTokens, uint16_t hwTokens);
static void repeat_msg_handler(msg_t *ptMsg);

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
    static function_key_evt_handler_t s_tSpecialKey = {START, &s_tRepeatByteEvent, &s_tRepeatLineEvent};
    static cmd_test_t s_tCmdTest4 = {START, &s_tFIFOout};
    static cmd_t s_tUserCmd[] = {&s_tCmdTest4, "test4", "    test4-just a test4\r\n", &test};
    const command_line_parsing_cfg_t c_tCmdParsingCFG = {UBOUND(s_tUserCmd), s_tUserCmd, &s_tFIFOout};
    static command_line_parsing_t s_tCmdParsing;
    console_cmd_init(&s_tCmdParsing, &c_tCmdParsingCFG);
    const static token_parsing_evt_handler_t c_tTokenParsingHandler = {&command_line_parsing, &s_tCmdParsing};
    static console_token_t s_tConsoleToken = {START, &s_tFIFOout, &c_tTokenParsingHandler};
    static uint8_t s_chBuffer[CONSOLE_BUFFER_SIZE + 1] = {'\0'};
    static uint8_t s_chLastBuffer[UBOUND(s_chBuffer)] = {'\0'};
    const static read_byte_evt_handler_t c_tReadByteEvent = {&dequeue_byte, &s_tFIFOConsolein};
    const static console_token_evt_handler_t c_tProcessingString = {&console_token, &s_tConsoleToken};
    const static console_frontend_cfg_t c_tConsoleCFG = {
                                        &c_tReadByteEvent,
                                        &c_tProcessingString,
                                        UBOUND(s_chBuffer),
                                        s_chBuffer,
                                        &s_tFIFOout,
                                        s_chLastBuffer,
                                        &s_tSpecialKey};
    static console_frontend_t s_tConsole;
    const static msg_t c_tMSGMap[] = {
                        {"\x1b\x4f\x50", &s_tRepeatByteEvent, &repeat_msg_handler},
                        {"\x1b\x4f\x51", NULL, NULL},
                        {"\x1b\x4f\x52", &s_tRepeatLineEvent, &repeat_msg_handler},
                        {"\x1b\x4f\x53", NULL, NULL},
                        {"\x1b\x4f\x54", NULL, NULL},
                        {"\x1b\x4f\x55", NULL, NULL},
                        {"\x1b\x4f\x56", NULL, NULL},
                        {"\x1b\x4f\x57", NULL, NULL},
                        {"\x1b\x4f\x58", NULL, NULL},
                        {"\x1b\x4f\x59", NULL, NULL},
                        {"\x1b\x4f\x5a", NULL, NULL},
                        {"\x1b\x4f\x5b", NULL, NULL},
                        {"\x1b\x4f\x5c", NULL, NULL},
                        {"\x1b\x5b\x41", NULL, NULL},
                        {"\x1b\x5b\x42", NULL, NULL},
                        {"\x1b\x5b\x43", NULL, NULL},
                        {"\x1b\x5b\x44", NULL, NULL},
                        {"\x1b\x5B\x31\x7E", NULL, NULL},
                        {"\x1b\x5B\x32\x7E", NULL, NULL},
                        {"\x1b\x5B\x33\x7E", NULL, NULL},
                        {"\x1b\x5B\x34\x7E", NULL, NULL},
                        {"\x1b\x5B\x35\x7E", NULL, NULL},
                        {"\x1b\x5B\x36\x7E", NULL, NULL}};
    const static check_msg_map_cfg_t c_tCheckMSGMapCFG = {
                                        UBOUND(c_tMSGMap), 
                                        &s_tFIFOin, 
                                        c_tMSGMap};
    static check_msg_map_t s_tCheckMSGMap;

    const static check_agent_t c_tCheckWordsAgent[] = {
                                {&s_tCheckMSGMap, check_msg_map}};
    const static check_use_peek_cfg_t c_tCheckWordsUsePeekCFG = {
                                        UBOUND(c_tCheckWordsAgent),
                                        &s_tFIFOin,
                                        (check_agent_t *)c_tCheckWordsAgent,
                                        &console_input};
    static check_use_peek_t s_tCheckWordsUsePeek;
    system_init();
    led_init();
    console_frontend_init(&s_tConsole, &c_tConsoleCFG);
    INIT_EVENT(&s_tRepeatByteEvent,false,false);
    INIT_EVENT(&s_tRepeatLineEvent,false,false);
    POOL_INIT(print_str, &s_tPrintFreeList);
    POOL_ADD_HEAP(print_str, &s_tPrintFreeList, s_chPrintStrPool, UBOUND(s_chPrintStrPool));
    INIT_BYTE_QUEUE(&s_tFIFOin, s_chBytein, sizeof(s_chBytein));
    INIT_BYTE_QUEUE(&s_tFIFOout, s_chByteout, sizeof(s_chByteout));
    INIT_BYTE_QUEUE(&s_tFIFOConsolein, s_chByteConsole, sizeof(s_chByteConsole));
    CHECK_MSG_MAP.Init(&s_tCheckMSGMap, &c_tCheckMSGMapCFG);
    CHECK_USE_PEEK.Init(&s_tCheckWordsUsePeek, &c_tCheckWordsUsePeekCFG);
    LED1_OFF();
    while (1) {
        breath_led();
        console_frontend(&s_tConsole);
        CHECK_USE_PEEK.CheckUsePeek(&s_tCheckWordsUsePeek);
        serial_in_task();
        serial_out_task();
    }
}
void repeat_msg_handler(msg_t *ptMsg)
{
    if (ptMsg != NULL) {
        if (ptMsg->pTarget != NULL) {
            SET_EVENT(ptMsg->pTarget);
        }
    }
}

bool console_input(uint8_t chByte)
{
    if (ENQUEUE_BYTE(&s_tFIFOConsolein, chByte)) {
        return true;
    }
    return false;
}

fsm_rt_t print_token(print_token_t *ptThis, uint8_t *pchTokens, uint16_t hwTokens)
{
    enum {
        START,
        CHECK_TOKEN_NUMBER,
        CHECK_STRING_END,
        OUTPUT_BYTE,
        END_STRING_ENTER,
        PRINT_TOKEN_ENTER
    };
    if ((NULL == ptThis) || (NULL == pchTokens)) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.pchTokensArray = pchTokens;
            this.hwTokens = hwTokens;
            this.chState = CHECK_TOKEN_NUMBER;
            // break;
        case CHECK_TOKEN_NUMBER:
            if (this.hwTokens > 0) {
                this.chState = CHECK_STRING_END;
            } else {
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            }
            // break;
        case CHECK_STRING_END:
        GOTO_CHECK_STRING_END:
            if (*this.pchTokensArray == '\0') {
                this.hwTokens--;
                this.pchTokensArray++;
                this.chState = END_STRING_ENTER;
                goto GOTO_END_STRING_ENTER;
            } else {
                this.chState = OUTPUT_BYTE;
            }
            // break;
        case OUTPUT_BYTE:
            if (print_str_output_byte(this.pTarget, *this.pchTokensArray++)) {
                this.chState = CHECK_STRING_END;
                goto GOTO_CHECK_STRING_END;
            }
            // break;
        case END_STRING_ENTER:
        GOTO_END_STRING_ENTER:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        ENTER, 
                        this.pTarget,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_TOKEN_ENTER;
                // break;
            }
        case PRINT_TOKEN_ENTER:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                this.chState = CHECK_TOKEN_NUMBER;
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