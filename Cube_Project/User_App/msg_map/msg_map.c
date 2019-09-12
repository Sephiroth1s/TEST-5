#include "app_cfg.h"
#define __MSG_MAP_CLASS_IMPLEMENT
#include "../queue/queue.h"
#include "../check_string/check_string.h"
#include "./msg_map.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define this (*ptThis)
#define TASK_RESET_FSM()      \
    do {                      \
        this.chState = START; \
    } while (0);

#define TASK_RESET_HANLDER_FSM() \
    do {                         \
        s_tState = START;        \
    } while (0);
#ifndef ASSERT
#   define ASSERT(...)
#endif

const i_check_msg_map_t CHECK_MSG_MAP{
    .Init = &check_msg_map_init,
    .Check = &check_msg_map,
};

// check_* only used in peek
bool check_msg_map_init(check_msg_map_t *ptObj, check_msg_map_cfg_t *ptCFG)
{
    /* initialise "this" (i.e. ptThis) to access class members */
    class_internal(ptObj, ptThis, check_msg_map_t);
    enum {
        START
    };
    ASSERT(NULL != ptObj && NULL != ptCFG);
    
    if ((NULL == ptCFG->use_as__msg_t) || (NULL == ptCFG->use_as__byte_queue_t)) {
        return false;
    }
    this.chState = START;
    this.chMSGNumber = ptCFG->chMSGNumber;
    this.use_as__byte_queue_t = ptCFG->use_as__byte_queue_t;
    this.use_as__msg_t = ptCFG->use_as__msg_t;
    return true;
}

fsm_rt_t check_msg_map(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop)
{
    enum {
        START,
        INIT_CHECK_MSG,
        CHECK_MSG,
        MSG_HANLDER,
        CHECK_MSG_NUMBER,
        DROP
    };
    class_internal(pTarget, ptThis, check_msg_map_t);
    enum {
        START
    };
    ASSERT(NULL != pTarget && NULL != ptCFG);
    switch (this.chState) {
        case START:
            this.chVoteDropCount = 0;
            this.chMSGCount = 0;
            this.chState = INIT_CHECK_MSG;
            // break;
        case INIT_CHECK_MSG:
            do {
                this.bIsRequestDrop = false;
                RESET_PEEK_BYTE(this.ptQueue);
                const check_str_cfg_t c_tCheckMSGCFG = {(this.use_as__msg_t[this.chMSGCount]).pchMessage,ptReadByte};
                check_string_init(&this.use_as__check_str_t, &c_tCheckMSGCFG);
            } while (0);
            this.chState = CHECK_MSG;
            //break;
        case CHECK_MSG:
            *pbRequestDrop = false;
            if (fsm_rt_cpl == check_string(&this.use_as__check_str_t, &this.bIsRequestDrop)) {
                this.chState = MSG_HANLDER;
                goto GOTO_MSG_HANLDER;
                break;
            }
            if (this.bIsRequestDrop) {
                this.chVoteDropCount++;
                this.bIsRequestDrop = false;
            }
            this.chState = CHECK_MSG_NUMBER;
            //break;
        case CHECK_MSG_NUMBER:
            this.chMSGCount++;
            if (this.chMSGCount >= this.chMSGNumber) {
                this.chMSGCount = 0;
                this.chState = DROP;
                //break;
            } else {
                this.chState = INIT_CHECK_MSG;
                break;
            }
        case DROP:
            if (this.chVoteDropCount >= this.chMSGNumber) {
                *pbRequestDrop = true;
            } else {
                *pbRequestDrop = false;
            }
            TASK_RESET_FSM();
            break;
        case MSG_HANLDER:
        GOTO_MSG_HANLDER:
            (this.use_as__msg_t[this.chMSGCount]).fnHandler(&this.use_as__msg_t[this.chMSGCount]);
            TASK_RESET_FSM();
            return fsm_rt_cpl;
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}