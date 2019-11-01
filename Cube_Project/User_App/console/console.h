#include "app_cfg.h"
#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "../print_string/print_string.h"
#include "../check_string/check_string.h"
#include "../msg_map/msg_map.h"
#include "../event/event.h"

typedef struct cmd_t cmd_t;
typedef fsm_rt_t cmd_handler_t(void *, uint8_t *, uint16_t);
struct cmd_t {
    void *pTarget;
    uint8_t *pchCmd;
    uint8_t *pchHelpInfo;
    cmd_handler_t *fnPrintToken;
};

typedef struct cmd_test_t cmd_test_t;  //  测试用 命令
struct cmd_test_t {
    uint8_t chState;
    void *pTarget;
    print_str_t *ptPrintStr;
};

typedef struct pring_all_help_info_t pring_all_help_info_t;  //  help 命令
struct pring_all_help_info_t {
    uint8_t chState;
    void *pTarget;
    uint8_t chDefaultCounter;
    uint8_t chUserCounter;
    uint8_t chCmdDefaultNumber;
    uint8_t chCmdUserNumber;
    cmd_t *ptUserCmd;
    cmd_t *ptDefaultCmd;
    print_str_t *ptPrintStr;
};

typedef struct clear_screen_t clear_screen_t;               // clear 命令
struct clear_screen_t {
    uint8_t chState;
    void *pTarget;
    print_str_t *ptPrintStr;
    uint16_t hwTokensCounter;
    uint8_t *pchCurrentTokens;
};

typedef struct command_line_parsing_t command_line_parsing_t;      // 分析token数组 处理命令
struct command_line_parsing_t{
    uint8_t chState;
    uint8_t chCmdUserNumber;
    uint8_t chCurrentUserCounter;
    cmd_t *ptUserCmd;
    cmd_t *ptDefaultCmd;
    uint8_t chCmdDefaultNumber;
    uint8_t chCurrentDefaultCmdCounter;
    uint16_t hwTokensCounter;
    uint8_t *pchCurrentTokens;
    cmd_t *ptCurrentTempCmd;
    void *pTarget;
    print_str_t *ptPrintStr;
};

typedef struct command_line_parsing_cfg_t command_line_parsing_cfg_t;  // 分析token 数组，注册命令配置宏
struct command_line_parsing_cfg_t{
    uint8_t chCmdNumber;
    cmd_t *ptUserCmd;
    void *pTarget;
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
    void *ptConsoleinTarget;
    console_token_evt_handler_t *ptConsoleToken;
    uint8_t chMaxNumber;
    uint8_t *pchCurrentBuffer;
    void *pOutputTarget;
} console_frontend_cfg_t;

extern bool console_frontend_init(console_frontend_t *ptThis, console_frontend_cfg_t *ptCFG);
extern fsm_rt_t console_frontend(console_frontend_t *ptThis);
#if VSF_USE_FUNCTION_KEY
static fsm_rt_t function_key(function_key_evt_handler_t *ptThis, uint8_t *chCurrentCounter, uint8_t *chLastMaxNumber);
#endif
static uint8_t *find_token(uint8_t *pchBuffer, uint16_t *hwTokens);
extern fsm_rt_t console_token(console_token_t *ptThis, uint8_t *pchBuffer);

extern bool console_cmd_init(command_line_parsing_t* ptThis, command_line_parsing_cfg_t *ptCFG);
extern fsm_rt_t command_line_parsing(command_line_parsing_t *ptThis, uint8_t *pchBuffer, uint16_t hwTokens); // 第一个token分析

static fsm_rt_t print_all_help_info(pring_all_help_info_t *ptThis, uint8_t *pchBuffer, uint16_t hwTokens);
static fsm_rt_t clear_screen(clear_screen_t *ptThis, uint8_t *pchBuffer, uint16_t hwTokens);
extern fsm_rt_t test(cmd_test_t *ptThis, uint8_t *pchBuffer, uint16_t hwTokens);

static void repeat_msg_handler(msg_t *ptMsg);
static bool console_frontend_input(uint8_t chByte);
extern bool console_task_init(byte_queue_t *pTarget);
extern void console_task(console_frontend_t *ptThis);

#endif
