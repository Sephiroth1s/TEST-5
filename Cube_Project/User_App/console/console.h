#include "app_cfg.h"
#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "../print_string/print_string.h"
#include "../check_string/check_string.h"
#include "../event/event.h"
#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc.h"
#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc_class.h"

#define __PLOOC_CLASS_USE_STRICT_TEMPLATE__
   
#if     defined(__CONSOLE_CLASS_IMPLEMENT)
#       define __PLOOC_CLASS_IMPLEMENT
#elif   defined(__CONSOLE_CLASS_INHERIT)
#       define __PLOOC_CLASS_INHERIT
#endif

declare_class(cmd_t);
declare_class(cmd_test_t);
declare_class(command_line_parsing_t);
declare_class(console_frontend_t);
declare_class(console_token_t);
declare_class(function_key_evt_handler_t);
declare_class(token_parsing_evt_handler_t);
declare_class(console_token_evt_handler_t);
typedef fsm_rt_t cmd_handler_t(void *, uint8_t *, uint16_t);
typedef fsm_rt_t token_parsing_handler_t(void *, uint8_t *, uint16_t);

def_class(cmd_t,
    private_member(
        void *pTarget;
        uint8_t *pchCmd;
        uint8_t *pchHelpInfo;
        cmd_handler_t *fnPrintToken;
    )
)
end_def_class(cmd_t)

def_class(cmd_test_t,
    private_member(
        uint8_t chState;
        void *pTarget;
        print_str_t *ptPrintStr;
    )
)
end_def_class(cmd_test_t)

def_class(command_line_parsing_t,
    private_member(
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
    )
)
end_def_class(command_line_parsing_t)

def_class(token_parsing_evt_handler_t,
    private_member(
        token_parsing_handler_t *fnPrintToken;
        void *pTarget;
    )
)
end_def_class(token_parsing_evt_handler_t)

def_class(console_token_t,
    private_member(
        uint8_t chState;
        void *pTarget;
        token_parsing_evt_handler_t *ptPrintToken;
        uint16_t hwTokens;
    )
)
end_def_class(console_token_t)

typedef fsm_rt_t console_token_handler_t(void *, uint8_t *);
def_class(console_token_evt_handler_t,
    private_member(
        console_token_handler_t *fnConsoleToken;
        void *pTarget;
    )
)
end_def_class(console_token_evt_handler_t)

def_class(console_frontend_t,
    private_member(
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
    )    
)
end_def_class(console_frontend_t)

typedef struct command_line_parsing_cfg_t command_line_parsing_cfg_t;  // 分析token 数组，注册命令配置宏
struct command_line_parsing_cfg_t{
    uint8_t chCmdNumber;
    cmd_t *ptUserCmd;
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
    console_token_evt_handler_t *ptConsoleToken;
    uint8_t chMaxNumber;
    uint8_t *pchCurrentBuffer;
    void *pOutputTarget;
} console_frontend_cfg_t;

typedef struct console_t console_t;
struct console_t {
    console_frontend_t *ptConsoleFrontend;
};

typedef struct console_cfg_t console_cfg_t;
struct console_cfg_t {
    read_byte_evt_handler_t *ptReadByteEVent;
    command_line_parsing_t *ptCmdParsing;
    console_frontend_t *ptConsoleFrontend;
    console_frontend_cfg_t *ptConsoleFrontendCFG;
    command_line_parsing_cfg_t *ptCmdParsingCFG;
};

extern bool console_task_init(console_t *ptObj, console_cfg_t *ptCFG);
extern fsm_rt_t console_frontend(console_frontend_t *ptObj);
extern fsm_rt_t console_token(console_token_t *ptObj, uint8_t *pchBuffer);
extern fsm_rt_t command_line_parsing(command_line_parsing_t *ptObj, uint8_t *pchBuffer, uint16_t hwTokens); 
extern fsm_rt_t test(cmd_test_t *ptObj, uint8_t *pchBuffer, uint16_t hwTokens);
extern void console_task(console_t *ptObj);


#endif
