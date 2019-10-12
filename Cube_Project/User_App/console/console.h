#include "app_cfg.h"
#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "../print_string/print_string.h"
#include "../check_string/check_string.h"
#include "../event/event.h"

typedef fsm_rt_t processing_string_t(void *, uint8_t *);
typedef struct processing_string_evt_handler_t processing_string_evt_handler_t;
struct processing_string_evt_handler_t {
    processing_string_t *fnProcessingString;
    void *pTarget;
};

typedef fsm_rt_t special_key_t();
typedef struct special_key_evt_handler_t special_key_evt_handler_t;
struct special_key_evt_handler_t {
    uint8_t chState;
    event_t *ptRepeatByte;
    event_t *ptRepeatLine;
    uint8_t chLastCounter;
    uint8_t chLastMaxNumber;
};

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
    uint8_t *pchLastBuffer;
    special_key_evt_handler_t *ptSpecialKey;
} console_print_t;

typedef struct {
    read_byte_evt_handler_t *ptReadByteEvent;
    processing_string_evt_handler_t *ptProcessingString;
    uint8_t chMaxNumber;
    uint8_t *pchCurrentBuffer;
    uint8_t *pchLastBuffer;
    void *pOutputTarget;
    special_key_evt_handler_t *ptSpecialKey;
} console_print_cfg_t;

extern bool task_console_init(console_print_t *ptThis, console_print_cfg_t *ptCFG);
extern fsm_rt_t task_console(console_print_t *ptThis);
static fsm_rt_t special_key_evt_handler(special_key_evt_handler_t *ptTarget,uint8_t *pchBuffer);

#endif
