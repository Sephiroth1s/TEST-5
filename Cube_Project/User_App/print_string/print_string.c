#include "app_cfg.h"
#define __PRINT_STR_CLASS_IMPLEMENT
#include "./print_string.h"
#include <stdint.h>
#include <stdbool.h>
#include "../queue/queue.h"
#include "../uart/uart.h"

#define TASK_STR_RESET_FSM()  \
    do {                      \
        this.chState = START; \
    } while (0);

#ifndef ASSERT
#   define ASSERT(...)
#endif

const i_print_str_t PRINT_STRING = {
    .Init = &print_string_init,
    .Print = &print_string,
};

IMPLEMENT_POOL(print_str, print_str_t);
WEAK bool print_str_output_byte(void *ptThis, uint8_t pchByte);

bool print_string_init(print_str_t *ptObj, const print_str_cfg_t *ptCFG)
{
    /* initialise "this" (i.e. ptThis) to access class members */
    class_internal(ptObj, ptThis, print_str_t);
    enum {
        START
    };
    ASSERT(NULL != ptObj && NULL != ptCFG);
    
    if (   (NULL == ptCFG) 
        || (NULL == ptCFG->pTarget)) {
        return false;
    }
    this.chState = START;
    this.pchString = ptCFG->pchString;
    this.pTarget = ptCFG->pTarget;
    return true;
}

fsm_rt_t print_string(print_str_t *ptObj)
{
    /* initialise "this" (i.e. ptThis) to access class members */
    class_internal(ptObj, ptThis, print_str_t);
    enum {
        START,
        PRINT_CHECK,
        PRINT_STR
    };
    ASSERT(NULL != ptObj);
    
    if (NULL == this.pTarget) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chState = PRINT_CHECK;
            // break;
        case PRINT_CHECK:
            if ('\0' == *this.pchString) {
                TASK_STR_RESET_FSM();
                return fsm_rt_cpl;
            } else {
                this.chState = PRINT_STR;
            }
            // break;
        case PRINT_STR:
            if (print_str_output_byte(this.pTarget, *this.pchString)) {
                this.pchString++;
                this.chState = PRINT_CHECK;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

WEAK bool print_str_output_byte(void *ptThis, uint8_t pchByte)
{
    return serial_out(pchByte);
}
