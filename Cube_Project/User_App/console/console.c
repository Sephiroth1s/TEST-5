#include "app_cfg.h"
#include "./console.h"
#include "../print_string/print_string.h"
#include "../check_string/check_string.h"

#define CURSOR_RIGHT "\033[C"        // 光标右移 1 行
#define ENTER "\x0A\x0D"             // 换行
#define CLEAR_SCREEN "\033[2J"       // 清屏
#define ERASE_END_OF_LINE "\033[K"   // 清除从光标到行尾的内容
#define ERASE_LINE "\033[2K"         //  清楚当前行
#define NEW_MARK ">"                 // 恢复'标识符号
#define CURSOR_TO_HOME "\033[1~"     // 光标移动到行首
#define MOVE_CURSOR "\033[1~\033[C"  // 移动光标到第一个字符之后

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
            || (NULL == ptCFG->ptReadByteEvent->fnReadByte)) {
        return false;
    }
    this.chState = START;
    this.chMaxNumber = ptCFG->chMaxNumber;
    this.pchBuffer = ptCFG->pchBuffer;
    this.ptReadByteEvent = ptCFG->ptReadByteEvent;
    this.ptUpdateLineTarget.chState = START;
    this.pOutputTarget = ptCFG->pOutputTarget;
    this.ptUpdateLineTarget.pOutputTarget = ptCFG->pOutputTarget;
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
        INIT_PRINT_LINE,
        PRINT_BUFFER
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
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
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
            if (fsm_rt_cpl == update_line(&this.ptUpdateLineTarget)) {
                this.chState = INIT_PRINT_LINE;
                // break
            } else {
                break;
            }
        case INIT_PRINT_LINE:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        this.pchBuffer, 
                        this.pOutputTarget,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_BUFFER;
                // break;
            }
        case PRINT_BUFFER:
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
        NEW_CURSOR,
        PRINT_MARK
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
                        MOVE_CURSOR, 
                        this.pOutputTarget,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_RESUME;
                // break;
            }
        case PRINT_RESUME:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
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
                        ERASE_END_OF_LINE, 
                        this.pOutputTarget,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_CLEAR;
                // break;
            }
        case PRINT_CLEAR:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
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
                this.chState = NEW_CURSOR;
                // break;
            } else {
                break;
            }
        case NEW_CURSOR:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                        NEW_MARK, 
                        this.pOutputTarget,
                        &enqueue_byte
                    };
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_MARK;
                // break;
            }
        case PRINT_MARK:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
                // break;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}