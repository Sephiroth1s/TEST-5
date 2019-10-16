#include "app_cfg.h"
#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "../print_string/print_string.h"
#include "../check_string/check_string.h"
#include "../event/event.h"

typedef fsm_rt_t console_token_handler_t(void *pThis, uint8_t *pchTokens, uint16_t hwTokens);
typedef struct processing_string_evt_handler_t processing_string_evt_handler_t;
struct processing_string_evt_handler_t {
    console_token_handler_t *fnProcessingString;
    void *pTarget;
};

#if VSF_USE_FUNCTION_KEY
typedef struct special_key_evt_handler_t special_key_evt_handler_t;
typedef fsm_rt_t special_key_function_t(special_key_evt_handler_t *ptThis, uint8_t *chCurrentCounter, uint8_t *chLastMaxNumber);
struct special_key_evt_handler_t {
    uint8_t chState;
    event_t *ptRepeatByte;
    event_t *ptRepeatLine;
    uint8_t chLastCounter;
    print_str_t *ptPrintStr;
    uint8_t *pchLastBuffer;
    uint8_t *pchCurrentBuffer;
    void *pOutputTarget;
    special_key_function_t *fnSpecialKey;
};
#endif

typedef struct {
    uint8_t chState;
    read_byte_evt_handler_t *ptReadByteEvent;
    processing_string_evt_handler_t *ptProcessingString;
    void *pOutputTarget;
    print_str_t *ptPrintStr;
    uint8_t *pchCurrentBuffer;
    uint8_t chByte;
    uint8_t chCurrentCounter;
    uint8_t chMaxNumber;
#if VSF_USE_FUNCTION_KEY
    uint8_t *pchLastBuffer;
    uint8_t chLastMaxNumber;
    special_key_evt_handler_t *ptSpecialKey;
#endif
} console_print_t;

typedef struct {
    read_byte_evt_handler_t *ptReadByteEvent;
    processing_string_evt_handler_t *ptProcessingString;
    uint8_t chMaxNumber;
    uint8_t *pchCurrentBuffer;
    void *pOutputTarget;
#if VSF_USE_FUNCTION_KEY
    uint8_t *pchLastBuffer;
    special_key_evt_handler_t *ptSpecialKey;
#endif
} console_print_cfg_t;

extern bool task_console_init(console_print_t *ptThis, console_print_cfg_t *ptCFG);
extern fsm_rt_t task_console(console_print_t *ptThis);
#if VSF_USE_FUNCTION_KEY
static fsm_rt_t special_key(special_key_evt_handler_t *ptThis, uint8_t *chCurrentCounter, uint8_t *chLastMaxNumber);
#endif
static uint8_t* string_token(uint8_t *pchBuffer,uint8_t *pchSeperators);

#endif
