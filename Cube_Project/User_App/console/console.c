#include "app_cfg.h"
#include "./console.h"
#include "../print_string/print_string.h"
#include "../check_string/check_string.h"

#define CURSOR_RIGHT "\033[C"        // 光标右移 1 行
#define ENTER "\x0A\x0D"             // 换行并输出标识符
#define ERASE_LINE "\033[2K"         //  清楚当前行
#define ENTER_AND_NEXT "\x0A\x0D>"   // 换行并且输出新行输入标识符

#define this (*ptThis)
#define TASK_CONSOLE_RESET_FSM() \
    do {                         \
        this.chState = START;    \
    } while (0);

POOL(print_str) s_tPrintFreeList;

bool task_console_init(console_print_t *ptThis,console_print_cfg_t *ptCFG)
{
    enum { 
        START 
    };
    if (       (NULL == ptThis) 
            || (NULL == ptCFG) 
            || (NULL == ptCFG->pchBuffer) 
            || (NULL == ptCFG->pOutputTarget)
            || (NULL == ptCFG->ptReadByteEvent) 
            || (NULL == ptCFG->ptReadByteEvent->fnReadByte)
            || (NULL == ptCFG->ptProcessingString)
            || (NULL == ptCFG->ptProcessingString->fnProcessingString)) {
        return false;
    }
    this.chState = START;
    this.chMaxNumber = ptCFG->chMaxNumber;
    this.pchBuffer = ptCFG->pchBuffer;
    this.ptReadByteEvent = ptCFG->ptReadByteEvent;
    this.pOutputTarget = ptCFG->pOutputTarget;
    this.ptProcessingString = ptCFG->ptProcessingString;
    return true;
}

fsm_rt_t task_console(console_print_t *ptThis)
{
    enum {
        START,
        READ_BYTE,
        CHECK_BYTE,
        CHECK_ENTER,
        CHECK_DELETE,
        WRITE_BUFFER,
        APPEND_BYTE,
        UPDATE_LINE,
        DELETE_BYTE,
        PRINT_ENTER,
        PROCESSING_STRING,
        NEXT_INPUT,
        PRINT_NEXT
    };
    uint8_t *pchTemp;
    if (NULL == ptThis) {
        return fsm_rt_err;
    }
    if ((NULL == this.ptReadByteEvent) || (NULL == this.pchBuffer)) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chState = READ_BYTE;
            // break;
        case READ_BYTE:
            if (this.ptReadByteEvent->fnReadByte(this.ptReadByteEvent->pTarget,
                                                 &this.chByte)) {
                this.chState = CHECK_BYTE;
                // break;
            } else {
                break;
            }
        case CHECK_BYTE:
            if ((this.chByte > 31) && (this.chByte < 127)) {
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
                *pchTemp = '\0';
                this.chCounter++;
                this.chState = APPEND_BYTE;
                // break;
            } else {
                TASK_CONSOLE_RESET_FSM();
                break;
            }
        case APPEND_BYTE:
        GOTO_APPEND_BYTE:
            if (print_str_output_byte(this.pOutputTarget, this.chByte)) {
                this.chState = READ_BYTE;
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
                this.chState = APPEND_BYTE;
                goto GOTO_APPEND_BYTE;
            } else {
                TASK_CONSOLE_RESET_FSM();
                break;
            }
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
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        ENTER, 
                        this.pOutputTarget,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_ENTER;
                // break;
            }
        case PRINT_ENTER:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                this.chState = PROCESSING_STRING;
                // break;
            } else{
                break;
            }
        case PROCESSING_STRING:
            if (fsm_rt_cpl == this.ptProcessingString->fnProcessingString(
                                this.ptProcessingString->pTarget, 
                                this.pchBuffer)) {
                this.chState = NEXT_INPUT;
                this.chCounter =0;
                *this.pchBuffer = '\0';
                // break;
            } else {
                break;
            }
        case NEXT_INPUT:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        ENTER_AND_NEXT, 
                        this.pOutputTarget,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_NEXT;
                // break;
            }
        case PRINT_NEXT:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}
