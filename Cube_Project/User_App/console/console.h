#include "app_cfg.h"
#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "../print_string/print_string.h"
#include "../check_string/check_string.h"
#include "../event/event.h"

typedef struct cmd_t cmd_t;
typedef fsm_rt_t cmd_handler_t(cmd_t *);
struct cmd_t {
    uint8_t *pchCmd;
    uint8_t *pchHelpInfo;
    cmd_handler_t *fnPrintToken;      
};

typedef command_line_parsing_t command_line_parsing_t;
struct command_line_parsing_t{
    uint8_t chState;
    uint8_t chCmdNumber;
    uint8_t chCurrentCounter;
    cmd_t *ptCmd[2];
    uint8_t chCmdStaticNumber;
    uint8_t chCurrentStaticCmdCounter;
    cmd_t *ptCmdStatic;
    cmd_t *ptCurrentTempCmd;
};
typedef command_line_parsing_cfg_t command_line_parsing_cfg_t;
struct command_line_parsing_cfg_t{
    uint8_t chCmdNumber;
    cmd_t *ptCmd;
};

typedef fsm_rt_t token_parsing_handler_t(void *, uint8_t *, uint16_t);
typedef struct token_parsing_evt_handler_t token_parsing_evt_handler_t;
struct token_parsing_evt_handler_t {
    token_parsing_handler_t *fnPrintToken;      // token 数组处理函数
    void *pTarget;
};

typedef struct console_token_t console_token_t;
struct console_token_t {
    uint8_t chState;
    void *pTarget;
    token_parsing_evt_handler_t *ptPrintToken;
    uint16_t hwTokens;
};

typedef fsm_rt_t console_token_handler_t(void *, uint8_t *);
typedef struct console_token_evt_handler_t console_token_evt_handler_t;
struct console_token_evt_handler_t {
    console_token_handler_t *fnConsoleToken;
    void *pTarget;
};


#if VSF_USE_FUNCTION_KEY
typedef struct function_key_evt_handler_t function_key_evt_handler_t;
typedef fsm_rt_t function_key_function_t(function_key_evt_handler_t *ptThis, uint8_t *chCurrentCounter, uint8_t *chLastMaxNumber);
struct function_key_evt_handler_t {
    uint8_t chState;
    event_t *ptRepeatByte;
    event_t *ptRepeatLine;
    uint8_t chLastCounter;
    print_str_t *ptPrintStr;
    uint8_t *pchLastBuffer;
    uint8_t *pchCurrentBuffer;
    void *pOutputTarget;
    function_key_function_t *fnFunctionKey;
};
#endif

typedef struct {
    uint8_t chState;
    read_byte_evt_handler_t *ptReadByteEvent;
    console_token_evt_handler_t *ptConsoleToken; 
    void *pOutputTarget;
    print_str_t *ptPrintStr;
    uint8_t *pchCurrentBuffer;
    uint8_t chByte;
    uint8_t chCurrentCounter;
    uint8_t chMaxNumber;
    uint16_t hwTokens;
#if VSF_USE_FUNCTION_KEY
    uint8_t *pchLastBuffer;
    uint8_t chLastMaxNumber;
    function_key_evt_handler_t *ptFunctionKey;
#endif
} console_frontend_t;

typedef struct {
    read_byte_evt_handler_t *ptReadByteEvent;
    console_token_evt_handler_t *ptConsoleToken;
    uint8_t chMaxNumber;
    uint8_t *pchCurrentBuffer;
    void *pOutputTarget;
#if VSF_USE_FUNCTION_KEY
    uint8_t *pchLastBuffer;
    function_key_evt_handler_t *ptFunctionKey;
#endif
} console_frontend_cfg_t;

extern bool console_frontend_init(console_frontend_t *ptThis, console_frontend_cfg_t *ptCFG);
extern fsm_rt_t console_frontend(console_frontend_t *ptThis);
#if VSF_USE_FUNCTION_KEY
static fsm_rt_t function_key(function_key_evt_handler_t *ptThis, uint8_t *chCurrentCounter, uint8_t *chLastMaxNumber);
#endif
static uint8_t *find_token(uint8_t *pchBuffer, uint16_t *hwTokens);
extern fsm_rt_t console_token(console_token_t *ptThis, uint8_t *pchBuffer);
#endif
