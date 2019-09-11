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

#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
#ifndef PRINT_STR_OUTPUT_BYTE
#error No defined macro PRINT_STR_OUTPUT_BYTE(__TARGET,__BYTE) for output byte, please define one with prototype bool (*)(void* pTarget,uint8_t chByte);
#endif
#else
#ifndef PRINT_STR_OUTPUT_BYTE
#error No defined macro PRINT_STR_OUTPUT_BYTE(__BYTE) for output byte, please define one with prototype bool (*)(uint8_t chByte);
#endif
#endif

const i_print_str_t PRINT_STRING = {
    .Init = &print_string_init,
    .Print = &print_string,
};

IMPLEMENT_POOL(print_str, print_str_t);

bool print_string_init(print_str_t *ptObj, const print_str_cfg_t *ptCFG)
{
    /* initialise "this" (i.e. ptThis) to access class members */
    class_internal(ptObj, ptThis, print_str_t);
    enum {
        START
    };
    ASSERT(NULL != ptObj && NULL != ptCFG);
    
    if (   (NULL == ptCFG) 
        || (NULL == ptCFG->pTarget)
        || (NULL == ptCFG->use_as__fn_print_byte_t)) {
        return false;
    }
    this.chState = START;
    this.pchString = ptCFG->pchString;
    this.pTarget = ptCFG->pTarget;
#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
    this.use_as__fn_print_byte_t = ptCFG->use_as__fn_print_byte_t;
#endif
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
    
    if ((this.use_as__fn_print_byte_t == NULL) || (NULL == this.pTarget)) {
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
#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
            if (PRINT_STR_OUTPUT_BYTE(this.pTarget, *this.pchString)) {
                this.pchString++;
                this.chState = PRINT_CHECK;
            }
#else
            if (PRINT_STR_OUTPUT_BYTE(*this.pchString)) {
                this.pchString++;
                this.chState = PRINT_CHECK;
            }
#endif
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}
