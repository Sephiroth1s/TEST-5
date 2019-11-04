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


extern POOL(print_str) s_tPrintFreeList;

static uint8_t s_chBytein[INPUT_FIFO_SIZE], s_chByteout[OUTPUT_FIFO_SIZE];
static byte_queue_t s_tFIFOin, s_tFIFOout;

static uint8_t s_chPrintStrPool[256] ALIGN(__alignof__(print_str_t));

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
    static read_byte_evt_handler_t s_tReadBytEvent={&dequeue_byte,&s_tFIFOin};
    static cmd_test_t s_tCmdTest4 = {START, &s_tFIFOout};
    static cmd_t s_tUserCmd[] = {&s_tCmdTest4, "test4", "    test4-just a test4\r\n", &test};
    const static command_line_parsing_cfg_t c_tCmdParsingCFG = {UBOUND(s_tUserCmd), s_tUserCmd, &s_tFIFOout};
    static command_line_parsing_t s_tCmdParsing;

    const static token_parsing_evt_handler_t c_tTokenParsingHandler = {&command_line_parsing, &s_tCmdParsing};
    static console_token_t s_tConsoleToken = {START, &s_tFIFOout, &c_tTokenParsingHandler};
    static uint8_t s_chBuffer[CONSOLE_BUFFER_SIZE + 1] = {'\0'};

    const static console_token_evt_handler_t c_tProcessingString = {&console_token, &s_tConsoleToken};
    const static console_frontend_cfg_t c_tConsoleCFG = {
                                        &c_tProcessingString,
                                        UBOUND(s_chBuffer),
                                        s_chBuffer,
                                        &s_tFIFOout};
    static console_frontend_t s_tConsoleFrontend;
    static console_cfg_t s_ConsoleCFG = {
                                        &s_tReadBytEvent, 
                                        &s_tCmdParsing,
                                        &s_tConsoleFrontend, 
                                        &c_tConsoleCFG,
                                        &c_tCmdParsingCFG};
    static console_t s_tConsole;
    system_init();
    led_init();
    console_task_init(&s_tConsole,&s_ConsoleCFG);
    POOL_INIT(print_str, &s_tPrintFreeList);
    POOL_ADD_HEAP(print_str, &s_tPrintFreeList, s_chPrintStrPool, UBOUND(s_chPrintStrPool));
    INIT_BYTE_QUEUE(&s_tFIFOin, s_chBytein, sizeof(s_chBytein));
    INIT_BYTE_QUEUE(&s_tFIFOout, s_chByteout, sizeof(s_chByteout));
    LED1_OFF();
    while (1) {
        breath_led();
        console_task(&s_tConsole);
        serial_in_task();
        serial_out_task();
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