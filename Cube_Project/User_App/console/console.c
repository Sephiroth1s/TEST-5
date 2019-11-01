#include "app_cfg.h"
#include "./console.h"
#include "../print_string/print_string.h"
#include "../check_string/check_string.h"
#include "../check_use_peek/check_use_peek.h"
#include "../msg_map/msg_map.h"
#include "../event/event.h"
#include <string.h>

#define CONSOLE_SEPERATORS " ;,-/"
#define CURSOR_RIGHT "\033[C"        // 光标右移 1 行
#define ENTER "\x0A\x0D"             // 换行
#define ERASE_LINE "\033[2K"         //  清楚当前行
#define CLEAR "\033c"              // 清屏

#define this (*ptThis)
#define TASK_CONSOLE_RESET_FSM() \
    do {                         \
        this.chState = START;    \
    } while (0);

#ifdef SUPPORT_CONSOLE_DEFAULT_CMD_EX_EN
#define CONSOLE_DEFAULT_CMD s_tDefaultCmd
#else
#ifndef CONSOLE_DEFAULT_CMD
#warning No defined macro SUPPORT_CONSOLE_DEFAULT_CMD_EX_EN for extended command, default two cmd is used.
#define CONSOLE_DEFAULT_CMD s_tDefaultCmd
#endif
#endif

POOL(print_str) s_tPrintFreeList;
#define CONSOLE_BUFFER_SIZE 50
#define CONSOLE_INPUT_SIZE 10
static check_use_peek_t s_tCheckWordsUsePeek;
static uint8_t s_chByteConsoleFrontendin[CONSOLE_INPUT_SIZE];
static byte_queue_t s_tFIFOConsoleFrontendin;
#if VSF_USE_FUNCTION_KEY
static uint8_t s_chLastBuffer[CONSOLE_BUFFER_SIZE+1] = {'\0'}; 
static event_t s_tRepeatLineEvent,s_tRepeatByteEvent;
static read_byte_evt_handler_t s_tReadByteEvent;
static function_key_evt_handler_t s_tFunctionKey;
#endif
static pring_all_help_info_t s_tPrintAllHelpInfo;
static clear_screen_t s_tClearScreen;
static cmd_test_t s_tCmdTest1;
static cmd_test_t s_tCmdTest2;
static cmd_test_t s_tCmdTest3;
static cmd_t s_tDefaultCmd[]={
                {&s_tPrintAllHelpInfo,"help","    help-Get command list of all available commands\r\n",&print_all_help_info},
                {&s_tClearScreen,"clear","    clear-Clear the screen\r\n",&clear_screen},
                #ifdef SUPPORT_CONSOLE_DEFAULT_CMD_EX_EN
                {&s_tCmdTest1,"test1","    test1-just a test1\r\n",&test},
                {&s_tCmdTest2,"test2","    test2-just a test2\r\n",&test},
                {&s_tCmdTest3,"test3","    test3-just a test3\r\n",&test}
                #endif
                };

bool console_task_init(void *pTarget)
{
    enum {
        START
    };
    INIT_BYTE_QUEUE(&s_tFIFOConsoleFrontendin, s_chByteConsoleFrontendin, sizeof(s_chByteConsoleFrontendin));
    #if VSF_USE_FUNCTION_KEY
    s_tReadByteEvent.fnReadByte = &dequeue_byte;
    s_tReadByteEvent.pTarget = pTarget;
    #endif
    const static msg_t c_tMSGMap[] = {
                        #if VSF_USE_FUNCTION_KEY
                        {"\x1b\x4f\x50", &s_tRepeatByteEvent, &repeat_msg_handler},
                        {"\x1b\x4f\x52", &s_tRepeatLineEvent, &repeat_msg_handler},
                        #else
                        {"\x1b\x4f\x50", NULL, NULL},
                        {"\x1b\x4f\x52", NULL, NULL},
                        #endif
                        {"\x1b\x4f\x51", NULL, NULL},
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
    static check_msg_map_cfg_t s_tCheckMSGMapCFG;
    s_tCheckMSGMapCFG.chMSGNumber = UBOUND(c_tMSGMap);
    s_tCheckMSGMapCFG.ptQueue = pTarget;
    s_tCheckMSGMapCFG.ptMSGMap = c_tMSGMap;
    static check_msg_map_t s_tCheckMSGMap;

    const static check_agent_t c_tCheckWordsAgent[] = {{&s_tCheckMSGMap, &check_msg_map}};
    static check_use_peek_cfg_t s_tCheckWordsUsePeekCFG;
    s_tCheckWordsUsePeekCFG.chAgentsNumber = UBOUND(c_tCheckWordsAgent);
    s_tCheckWordsUsePeekCFG.ptQueue = pTarget;
    s_tCheckWordsUsePeekCFG.ptAgents = (check_agent_t *)c_tCheckWordsAgent;
    s_tCheckWordsUsePeekCFG.fnOnDropByte = &console_frontend_input;

    CHECK_MSG_MAP.Init(&s_tCheckMSGMap, &s_tCheckMSGMapCFG);
    CHECK_USE_PEEK.Init(&s_tCheckWordsUsePeek, &s_tCheckWordsUsePeekCFG);
}

void console_task(console_frontend_t *ptThis)
{
    CHECK_USE_PEEK.CheckUsePeek(&s_tCheckWordsUsePeek);
    console_frontend(ptThis);
}

bool console_frontend_init(console_frontend_t *ptThis,console_frontend_cfg_t *ptCFG)
{
    enum { 
        START 
    };
    if (       (NULL == ptThis) 
            || (NULL == ptCFG) 
            || (NULL == ptCFG->pchCurrentBuffer) 
            || (NULL == ptCFG->pOutputTarget)
            || (NULL == ptCFG->ptConsoleToken)
            || (NULL == ptCFG->ptConsoleToken->fnConsoleToken)) {
        return false;
    }

#if VSF_USE_FUNCTION_KEY
    INIT_EVENT(&s_tRepeatByteEvent,false,false);
    INIT_EVENT(&s_tRepeatLineEvent,false,false);
    s_tFunctionKey.chState = START;
    s_tFunctionKey.ptRepeatByte = &s_tRepeatByteEvent;
    s_tFunctionKey.ptRepeatLine = &s_tRepeatLineEvent;
    this.pchLastBuffer = s_chLastBuffer;
    this.chLastMaxNumber = 0;
    this.ptFunctionKey = &s_tFunctionKey;
    this.ptFunctionKey->chLastCounter = 0;
    this.ptFunctionKey->pOutputTarget = ptCFG->pOutputTarget;
    this.ptFunctionKey->pchLastBuffer = s_chLastBuffer;
    this.ptFunctionKey->pchCurrentBuffer = ptCFG->pchCurrentBuffer;
    this.ptFunctionKey->fnFunctionKey = &function_key;
#endif
    static read_byte_evt_handler_t s_tReadByteFrontend;
    s_tReadByteFrontend.fnReadByte = &dequeue_byte;
    s_tReadByteFrontend.pTarget = &s_tFIFOConsoleFrontendin;
    this.chState = START;
    this.chMaxNumber = ptCFG->chMaxNumber-1;
    this.pchCurrentBuffer = ptCFG->pchCurrentBuffer;
    this.ptReadByteEvent = &s_tReadByteFrontend;
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

bool console_cmd_init(command_line_parsing_t* ptThis, command_line_parsing_cfg_t *ptCFG)
{
    enum {
        START
    };
    if ((NULL == ptThis) || (NULL == ptCFG)) {
        return false;
    }
    s_tPrintAllHelpInfo.chState = START;
    s_tPrintAllHelpInfo.pTarget = ptCFG->pTarget;
    s_tPrintAllHelpInfo.ptDefaultCmd = CONSOLE_DEFAULT_CMD;
    s_tPrintAllHelpInfo.ptUserCmd = ptCFG->ptUserCmd;
    s_tPrintAllHelpInfo.chCmdDefaultNumber = UBOUND(CONSOLE_DEFAULT_CMD);
    s_tPrintAllHelpInfo.chCmdUserNumber = ptCFG->chCmdNumber;

    s_tClearScreen.chState = START;
    s_tClearScreen.pTarget = ptCFG->pTarget;
    #ifdef SUPPORT_CONSOLE_DEFAULT_CMD_EX_EN
    s_tCmdTest1.chState = START;
    s_tCmdTest1.pTarget = ptCFG->pTarget;
    s_tCmdTest2.chState = START;
    s_tCmdTest2.pTarget = ptCFG->pTarget;
    s_tCmdTest3.chState = START;
    s_tCmdTest3.pTarget = ptCFG->pTarget;
    #endif
    if (NULL == ptCFG->ptUserCmd) {
        this.chCmdUserNumber = 0;
        this.ptUserCmd = NULL;
    } else {
        this.chCmdUserNumber = ptCFG->chCmdNumber;
        this.ptUserCmd = ptCFG->ptUserCmd;
    }
    this.chState = START;
    this.chCmdDefaultNumber = UBOUND(CONSOLE_DEFAULT_CMD);
    this.ptDefaultCmd = CONSOLE_DEFAULT_CMD;
    this.pTarget = ptCFG->pTarget;
    return true;
}

fsm_rt_t command_line_parsing(command_line_parsing_t *ptThis, uint8_t *pchBuffer, uint16_t hwTokens)
{
    enum {
        START,
        CHECK_TOKEN1_DEFAULT,
        CHECK_CMD_DEFAULT,
        CHECK_USER_CMD,
        CHECK_TOKEN1_USER,
        CHECK_CMD_USER,
        CHECK_TOKEN_IS_EMPTY,
        CHECK_TOKEN_HELP,
        PRINT_HELP_INFO_INIT,
        PRINT_HELP_INFO,
        CMD_HANDLER
    };
    if ((NULL == ptThis) || (NULL == pchBuffer)) {
        return fsm_rt_err;
    }
    if (NULL == this.ptDefaultCmd) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chCurrentDefaultCmdCounter = 0;
            this.hwTokensCounter = 0;
            this.pchCurrentTokens = pchBuffer;
            this.chState = CHECK_TOKEN1_DEFAULT;
            // break;
        case CHECK_TOKEN1_DEFAULT:
        GOTO_CHECK_TOKEN1_DEFAULT:
            if (this.chCurrentDefaultCmdCounter < this.chCmdDefaultNumber) {
                this.chState = CHECK_CMD_DEFAULT;
            } else {
                this.chState = CHECK_USER_CMD;
                goto GOTO_CHECK_USER_CMD;
            }
            // break;
        case CHECK_CMD_DEFAULT:
            if (strcmp(this.pchCurrentTokens,
                       this.ptDefaultCmd[this.chCurrentDefaultCmdCounter].pchCmd) == 0) {
                this.ptCurrentTempCmd = &this.ptDefaultCmd[this.chCurrentDefaultCmdCounter];
                this.hwTokensCounter++;
                this.chState = CHECK_TOKEN_IS_EMPTY;
            } else {
                this.chCurrentDefaultCmdCounter++;
                this.chState = CHECK_TOKEN1_DEFAULT;
                goto GOTO_CHECK_TOKEN1_DEFAULT;
            }
            break;
        case CHECK_TOKEN_IS_EMPTY:
        GOTO_CHECK_TOKEN_IS_EMPTY:
            if (this.hwTokensCounter < hwTokens) {
                this.pchCurrentTokens = this.pchCurrentTokens + strlen(this.pchCurrentTokens) + 1;
                this.chState = CHECK_TOKEN_HELP;
            }else{
                this.chState=CMD_HANDLER;  
                goto GOTO_CMD_HANDLER;
            }
            // break;
        case CHECK_TOKEN_HELP:
            if (strcmp("help", this.pchCurrentTokens) == 0 
                || strcmp("h", this.pchCurrentTokens) == 0) {
                this.chState = PRINT_HELP_INFO_INIT;
            }else{
                this.chState = CHECK_TOKEN_IS_EMPTY;
                goto GOTO_CHECK_TOKEN_IS_EMPTY;
            }
            // break;
        case PRINT_HELP_INFO_INIT:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                                            this.ptCurrentTempCmd->pchHelpInfo, 
                                            this.pTarget,
                                            &enqueue_byte};
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = PRINT_HELP_INFO;
                // break;
            }
        case PRINT_HELP_INFO:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        case CMD_HANDLER:
        GOTO_CMD_HANDLER:
            if (fsm_rt_cpl == this.ptCurrentTempCmd->fnPrintToken(
                                  this.ptCurrentTempCmd->pTarget, 
                                  pchBuffer,
                                  hwTokens)) {
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        case CHECK_USER_CMD:
        GOTO_CHECK_USER_CMD:
            if (this.ptUserCmd == NULL) {
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            } else {
                this.chCurrentUserCounter = 0;
                this.chState = CHECK_TOKEN1_USER;
            }
            // break;
        case CHECK_TOKEN1_USER:
        GOTO_CHECK_TOKEN1_USER:
            if (this.chCurrentUserCounter < this.chCmdUserNumber) {
                this.chState = CHECK_USER_CMD;
            } else {
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            }
            // break;
        case CHECK_CMD_USER:
            if (strcmp(this.pchCurrentTokens, this.ptUserCmd[this.chCurrentUserCounter].pchCmd) == 0) {
                this.ptCurrentTempCmd = &this.ptUserCmd[this.chCurrentUserCounter];
                this.hwTokensCounter++;
                this.chState = CHECK_TOKEN_IS_EMPTY;
            } else {
                this.chCurrentUserCounter++;
                this.chState = CHECK_TOKEN1_USER;
                goto GOTO_CHECK_TOKEN1_USER;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

fsm_rt_t print_all_help_info(pring_all_help_info_t *ptThis, uint8_t *pchBuffer, uint16_t hwTokens)
{
    enum {
        START,
        PRING_DEFAULT_HELP_INFO_INIT,
        PRITN_DEFAULT_HELP_INFO,
        CHECK_DEFAUTL_INFO_COMPLETE,
        CHECK_USER_CMD,
        PRINT_USER_CMD_HELP_INFO_INIT,
        PRINT_USER_CMD_HELP_INFO,
        CHECK_USER_INFO_COMPLETE
    };
    if (NULL == ptThis) {
        return fsm_rt_err;
    }
    if (NULL == this.pTarget) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chDefaultCounter = 0;
            this.chState = PRING_DEFAULT_HELP_INFO_INIT;
            // break;
        case CHECK_DEFAUTL_INFO_COMPLETE:
            if (this.chDefaultCounter < this.chCmdDefaultNumber) {
                this.chState = PRING_DEFAULT_HELP_INFO_INIT;
            } else{
                this.chState = CHECK_USER_CMD;
                goto GOTO_CHECK_USER_CMD;
            }
            // break;
        case PRING_DEFAULT_HELP_INFO_INIT:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                                    this.ptDefaultCmd[this.chDefaultCounter].pchHelpInfo,
                                    this.pTarget,
                                    &enqueue_byte};
                    PRINT_STRING.Init(this.ptPrintStr,&c_tCFG);
                } while (0);
                this.chState=PRITN_DEFAULT_HELP_INFO;
            }
            // break;
        case PRITN_DEFAULT_HELP_INFO:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                this.chDefaultCounter++;
                this.chState = CHECK_DEFAUTL_INFO_COMPLETE;
            }
            break;
        case CHECK_USER_CMD:
        GOTO_CHECK_USER_CMD:
            if (this.ptUserCmd == NULL) {
                TASK_CONSOLE_RESET_FSM();
                return fsm_rt_cpl;
            } else {
                this.chUserCounter = 0;
                this.chState = PRINT_USER_CMD_HELP_INFO_INIT;
            }
            break;
        case PRINT_USER_CMD_HELP_INFO_INIT:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                                    this.ptUserCmd[this.chUserCounter].pchHelpInfo,
                                    this.pTarget,
                                    &enqueue_byte};
                    PRINT_STRING.Init(this.ptPrintStr,&c_tCFG);
                } while (0);
                this.chState = PRINT_USER_CMD_HELP_INFO;
            }
            // break;
        case PRINT_USER_CMD_HELP_INFO:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                this.chUserCounter++;
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                this.chState = CHECK_USER_INFO_COMPLETE;
            }
            break;
        case CHECK_USER_INFO_COMPLETE:
            if (this.chUserCounter < this.chCmdUserNumber) {
                this.chState = PRINT_USER_CMD_HELP_INFO_INIT;
            } else{
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

fsm_rt_t clear_screen(clear_screen_t *ptThis, uint8_t *pchBuffer, uint16_t hwTokens)
{
     enum { 
        START,
        CLEAR_SCREEN_INIT,
        CLEAR_SCREEN
    };
    if (NULL == ptThis) {
        return fsm_rt_err;
    }
    if (NULL == this.pTarget) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chState = CLEAR_SCREEN_INIT;
            // break;
        case CLEAR_SCREEN_INIT:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (this.ptPrintStr == NULL) {
                break;
            } else {
                do {
                    const print_str_cfg_t c_tCFG = {
                                            CLEAR, 
                                            this.pTarget,
                                            &enqueue_byte};
                    PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
                } while (0);
                this.chState = CLEAR_SCREEN;
                // break;
            }
        case CLEAR_SCREEN:
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

fsm_rt_t test(cmd_test_t *ptThis, uint8_t *pchBuffer, uint16_t hwTokens)
{
    printf("    Current cmd is %s\r\n",pchBuffer);
    return fsm_rt_cpl;
}

void repeat_msg_handler(msg_t *ptMsg)
{
    if (ptMsg != NULL) {
        if (ptMsg->pTarget != NULL) {
            SET_EVENT(ptMsg->pTarget);
        }
    }
}

bool console_frontend_input(uint8_t chByte)
{
    if (ENQUEUE_BYTE(&s_tFIFOConsoleFrontendin, chByte)) {
        return true;
    }
    return false;
}