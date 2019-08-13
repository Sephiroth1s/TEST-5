#include "app_cfg.h"
#include "../queue/queue.h"
#include "../check_string/check_string.h"
#include "../msg_map/msg_map.h"
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

bool search_msg_map_init(search_msg_map_t *ptThis, search_msg_map_cfg_t *ptCFG)
{
    enum {
        START
    };
    if ((NULL == ptCFG)) {
        if (    (NULL == ptThis) 
            ||  (NULL == ptCFG->ptMSGMap)
            ||  (NULL == ptCFG->ptQueue)){
                return false;
            }
    }
    this.chState = START;
    this.chMSGNumber = ptCFG->chMSGNumber;
    this.ptQueue = ptCFG->ptQueue;
    this.ptReadByteEvent.pTarget = this.ptQueue;
    this.ptReadByteEvent.fnReadByte = (read_byte_t*)&peek_byte_queue;
    this.ptMSGMap = ptCFG->ptMSGMap;
    return true;
}

msg_t *search_msg_map(search_msg_map_t *ptThis)
{
    enum {
        START,
        INIT_CHECK_MSG,
        CHECK_MSG,
        CHECK_MSG_NUMBER,
        DROP
    };
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
                const check_str_cfg_t c_tCheckMSGCFG = {(this.ptMSGMap[this.chMSGCount]).pchMessage, &this.ptReadByteEvent};
                check_string_init(&this.tCheckMSG, &c_tCheckMSGCFG);
            } while (0);
            this.chState = CHECK_MSG;
            //break;
        case CHECK_MSG:
            if (fsm_rt_cpl == check_string(&this.tCheckMSG, &this.bIsRequestDrop)) {
                GET_ALL_PEEKED_BYTE(this.ptQueue);
                TASK_RESET_FSM();
                return &this.ptMSGMap[this.chMSGCount];
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
                do {
                    uint8_t chByteDrop;
                    DEQUEUE_BYTE(this.ptQueue, &chByteDrop);
                } while (0);
                RESET_PEEK_BYTE(this.ptQueue);
            }
            TASK_RESET_FSM();
            break;
        default:
            return NULL;
            break;
    }
    return NULL;
}

void msg_map_hanlder(msg_t*ptThis)
{
    enum {
        START,
        FIND_MSG,
        MSG_HANLDER
    } s_tState = START;
    switch (s_tState) {
        case START:
            s_tState = FIND_MSG;
            //break;
        case FIND_MSG:
            if (NULL != ptThis) {
                s_tState = MSG_HANLDER;
                //break;
            } else {
                break;
            }
        case MSG_HANLDER:
        GOTO_MSG_HANLDER:
            if (fsm_rt_cpl == this.fnHandler(ptThis)) {
                TASK_RESET_HANLDER_FSM();
            } else {
                goto GOTO_MSG_HANLDER;
            }
            break;
        default:
            break;
    }
}
