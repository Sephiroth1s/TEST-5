#include "app_cfg.h"
#include "./console.h"
#include "../print_string/print_string.h"
#include "../check_string/check_string.h"
#include "../event/event.h"
#include <string.h>

#define CONSOLE_SEPERATORS " ;,-/"
#define CURSOR_RIGHT "\033[C"        // 光标右移 1 行
#define ENTER "\x0A\x0D"             // 换行
#define ERASE_LINE "\033[2K"         //  清楚当前行

#define this (*ptThis)
#define TASK_CONSOLE_RESET_FSM() \
    do {                         \
        this.chState = START;    \
    } while (0);

POOL(print_str) s_tPrintFreeList;

bool console_frontend_init(console_frontend_t *ptThis,console_frontend_cfg_t *ptCFG)
{
    enum { 
        START 
    };
    if (       (NULL == ptThis) 
            || (NULL == ptCFG) 
            || (NULL == ptCFG->pchCurrentBuffer) 
            || (NULL == ptCFG->pOutputTarget)
            || (NULL == ptCFG->ptReadByteEvent) 
            || (NULL == ptCFG->ptReadByteEvent->fnReadByte)
            || (NULL == ptCFG->ptConsoleToken)
            || (NULL == ptCFG->ptConsoleToken->fnConsoleToken)) {
        return false;
    }

#if VSF_USE_FUNCTION_KEY
    if(        (NULL == ptCFG->ptFunctionKey)
            || (NULL == ptCFG->ptFunctionKey->ptRepeatByte)
            || (NULL == ptCFG->ptFunctionKey->ptRepeatLine)){
        return false;
    }
    this.pchLastBuffer = ptCFG->pchLastBuffer;
    this.chLastMaxNumber = 0;
    this.ptFunctionKey = ptCFG->ptFunctionKey;
    this.ptFunctionKey->chLastCounter = 0;
    this.ptFunctionKey->pOutputTarget = ptCFG->pOutputTarget;
    this.ptFunctionKey->pchLastBuffer = ptCFG->pchLastBuffer;
    this.ptFunctionKey->pchCurrentBuffer = ptCFG->pchCurrentBuffer;
    this.ptFunctionKey->fnFunctionKey = &function_key;
#endif

    this.chState = START;
    this.chMaxNumber = ptCFG->chMaxNumber-1;
    this.pchCurrentBuffer = ptCFG->pchCurrentBuffer;
    this.ptReadByteEvent = ptCFG->ptReadByteEvent;
    this.pOutputTarget = ptCFG->pOutputTarget;
    this.ptConsoleToken = ptCFG->ptConsoleToken;
    return true;
}

fsm_rt_t console_frontend(console_frontend_t *ptThis)
{
    enum {
        START,
        PRINT_START_FLAG,
        #if VSF_USE_FUNCTION_KEY
        VSF_USE_FUNCTION_KEY_F1_F3,
        #endif
        READ_BYTE,
        CHECK_BYTE,
        CHECK_ENTER,
        CHECK_DELETE,
        WRITE_BUFFER,
        APPEND_BYTE,
        UPDATE_LINE,
        IS_EMPTY,
        DELETE_BYTE,
        PRINT_ENTER,
        USER_HANDLER
    };
    uint8_t *pchTemp;
    if (NULL == ptThis) {
        return fsm_rt_err;
    }
    if ((NULL == this.ptReadByteEvent) || (NULL == this.pchCurrentBuffer)) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chCurrentCounter = 0;
            this.hwTokens = 0;
            *this.pchCurrentBuffer = '\0';
            this.chState = PRINT_START_FLAG;
            // break;
        case PRINT_START_FLAG:
            if (print_str_output_byte(this.pOutputTarget, '>')) {
                this.chCurrentCounter = 0;
                *this.pchCurrentBuffer = '\0';
                #if VSF_USE_FUNCTION_KEY
                this.chState = VSF_USE_FUNCTION_KEY_F1_F3;
                #else
                this.chState = READ_BYTE;
                goto READ_BYTE_START;
                #endif
                // break;
            } else {
                break;
            }
        #if VSF_USE_FUNCTION_KEY
        case VSF_USE_FUNCTION_KEY_F1_F3:
            if (fsm_rt_cpl == this.ptFunctionKey->fnFunctionKey(
                                  this.ptFunctionKey, 
                                  &this.chCurrentCounter,
                                  &this.chLastMaxNumber)) {
                this.chState = READ_BYTE;
                // break;
            } else {
                break;
            }
        #endif
        case READ_BYTE:
        READ_BYTE_START:
            if (this.ptReadByteEvent->fnReadByte(this.ptReadByteEvent->pTarget,
                                                 &this.chByte)) {
                this.chState = CHECK_BYTE;
                // break;
            } else {
                #if VSF_USE_FUNCTION_KEY
                this.chState = VSF_USE_FUNCTION_KEY_F1_F3;
                #else
                this.chState = READ_BYTE;
                #endif
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
            pchTemp = this.pchCurrentBuffer + this.chCurrentCounter;
            if (this.chCurrentCounter < this.chMaxNumber) {
                *pchTemp++ = this.chByte;
                *pchTemp = '\0';
                this.chCurrentCounter++;
                this.chState = APPEND_BYTE;
                // break;
            } else {
                #if VSF_USE_FUNCTION_KEY
                this.chState = VSF_USE_FUNCTION_KEY_F1_F3;
                #else
                this.chState = READ_BYTE;
                #endif
                break;
            }
        case APPEND_BYTE:
        GOTO_APPEND_BYTE:
            if (print_str_output_byte(this.pOutputTarget, this.chByte)) {
                #if VSF_USE_FUNCTION_KEY
                this.chState = VSF_USE_FUNCTION_KEY_F1_F3;
                #else
                this.chState = READ_BYTE;
                #endif
            }
            break;
        case CHECK_DELETE:
        GOTO_CHECK_DELETE:
            if (this.chByte == '\x7F') {
                this.chState = DELETE_BYTE;
                // break;
            } else {
                #if VSF_USE_FUNCTION_KEY
                this.chState = VSF_USE_FUNCTION_KEY_F1_F3;
                #else
                this.chState = READ_BYTE;
                #endif
                break;
            }
        case DELETE_BYTE:
            if (this.chCurrentCounter > 0) {
                this.chCurrentCounter--;
                pchTemp = this.pchCurrentBuffer + this.chCurrentCounter;
                *pchTemp = '\0';
                this.chState = APPEND_BYTE;
                goto GOTO_APPEND_BYTE;
            } else {
                #if VSF_USE_FUNCTION_KEY
                this.chState = VSF_USE_FUNCTION_KEY_F1_F3;
                #else
                this.chState = READ_BYTE;
                #endif
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
                this.chState = IS_EMPTY;
                // break;
            } else{
                break;
            }
        case IS_EMPTY:
            if (!this.chCurrentCounter) {
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            }
            #if VSF_USE_FUNCTION_KEY
            this.chLastMaxNumber = this.chCurrentCounter;
            memcpy(this.pchLastBuffer, this.pchCurrentBuffer, this.chLastMaxNumber + 1);
            #endif
            this.chState = USER_HANDLER;
            // break;
        case USER_HANDLER:
            if (fsm_rt_cpl == this.ptConsoleToken->fnConsoleToken(
                                this.ptConsoleToken->pTarget,
                                this.pchCurrentBuffer)) {
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

#if VSF_USE_FUNCTION_KEY
fsm_rt_t function_key(function_key_evt_handler_t *ptThis, uint8_t *chCurrentCounter, uint8_t *chLastMaxNumber)
{
    enum{
        START,
        KEY_F1,
        KEY_F3,
        IS_BEYOND_F1,
        IS_BEYOND_F3,
        REPEAT_BYTE,
        REPEAT_LINE,
        PRINT_LAST_CMD
    };
    switch (this.chState)
    {
        case START:
            this.chState = KEY_F1;
            // break;
        case KEY_F1:
            if (WAIT_EVENT(this.ptRepeatByte)) {
                this.chLastCounter = *chCurrentCounter;
                this.chState = IS_BEYOND_F1;
            } else {
                this.chState = KEY_F3;
                goto GOTO_KEY_F3;
            }
            // break;
        case IS_BEYOND_F1:
            if (this.chLastCounter >= *chLastMaxNumber) {
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            }
            this.chState = REPEAT_BYTE;
            // break;
        case REPEAT_BYTE:
            do {
                uint8_t *pchLastTemp, *pchTemp;
                pchLastTemp = this.pchLastBuffer + this.chLastCounter;
                if (print_str_output_byte(this.pOutputTarget, *pchLastTemp)) {
                    pchTemp = this.pchCurrentBuffer + *chCurrentCounter;
                    *pchTemp++ = *pchLastTemp;
                    *pchTemp = '\0';
                    this.chLastCounter++;
                    *chCurrentCounter = this.chLastCounter;
                    RESET_EVENT(this.ptRepeatByte);
                    this.chState = KEY_F3;
                }
            } while (0);
            break;
        case KEY_F3:
        GOTO_KEY_F3:
            if (WAIT_EVENT(this.ptRepeatLine)) {
                this.chLastCounter = *chCurrentCounter;
                this.chState = IS_BEYOND_F3;
            } else {
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            }
            // break;
        case IS_BEYOND_F3:
        GOTO_IS_BEYOND_F3:
            do {
                uint8_t *pchLastTemp, *pchTemp;
                if (this.chLastCounter >= *chLastMaxNumber) {
                    *chCurrentCounter = this.chLastCounter;
                    TASK_CONSOLE_RESET_FSM();
                    return fsm_rt_cpl;
                }
            } while (0);
            this.chState = REPEAT_LINE;
            // break;
        case REPEAT_LINE:
            do {
                uint8_t *pchLastTemp, *pchTemp;
                pchLastTemp = this.pchLastBuffer + this.chLastCounter;
                if (print_str_output_byte(this.pOutputTarget, *pchLastTemp)) {
                    pchTemp = this.pchCurrentBuffer + *chCurrentCounter;
                    *pchTemp++ = *pchLastTemp;
                    *pchTemp = '\0';
                    this.chLastCounter++;
                    *chCurrentCounter = this.chLastCounter;
                    this.chState = IS_BEYOND_F3;
                }
            } while (0);
            goto GOTO_IS_BEYOND_F3;
            // break;
        default:
            return fsm_rt_err;
    }
    return fsm_rt_on_going;
}
#endif

fsm_rt_t console_token(console_token_t *ptThis, uint8_t*pchBuffer)
{
    enum { 
        START, 
        FIND_TOKEN, 
        USER_HANDLER 
    };
    switch (this.chState) {
        case START:
            this.hwTokens = 0;
            this.chState = FIND_TOKEN;
            // break;
        case FIND_TOKEN:
            if (find_token(pchBuffer, &this.hwTokens) != NULL) {
                this.chState = USER_HANDLER;
            }
            break;
        case USER_HANDLER:
            if (fsm_rt_cpl == this.ptPrintToken->fnPrintToken(
                                            this.ptPrintToken->pTarget,
                                            pchBuffer,
                                            this.hwTokens)) {
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

uint8_t* find_token(uint8_t *pchBuffer, uint16_t *hwTokens)
{
    if (pchBuffer == NULL) {
        return NULL;
    }
    static uint8_t *pchTempSeperators;
    bool bFlag;
    while (*pchBuffer != '\0') {
        pchTempSeperators = CONSOLE_SEPERATORS;
        bFlag = false;
        while (*pchTempSeperators != '\0') {
            if (*pchTempSeperators++ == *pchBuffer) {
                bFlag = true;
                break;
            }
        }
        if (!bFlag) {
            break;
        }
        ++pchBuffer;
    }

    uint8_t *pchReadBuffer = pchBuffer;
    uint8_t *pchWriteBuffer = pchBuffer;
    uint8_t chCounter = 0;
    uint8_t chSizeBuffer = strlen(pchBuffer);
    for (uint8_t chBufferCounter = 0; chBufferCounter <= chSizeBuffer; chBufferCounter++) {
        pchTempSeperators = CONSOLE_SEPERATORS;
        bFlag = false;
        for (uint8_t chSeperatorsCounter = 0; chSeperatorsCounter <= strlen(CONSOLE_SEPERATORS); chSeperatorsCounter++) {
            if (*pchTempSeperators++ == *pchReadBuffer) {
                bFlag = true;
                break;
            }
        }
        if (bFlag) {
            chCounter++;
            if (chCounter == 1) {
                *pchWriteBuffer++ = '\0';
                *hwTokens = *hwTokens + 1;
            }
        } else {
            chCounter = 0;
            *pchWriteBuffer = *pchReadBuffer;
            pchWriteBuffer++;
        }
        pchReadBuffer++;
    }
    return pchBuffer;
}