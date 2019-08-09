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

#define TASK_CHECK_RESET_FSM() \
    do {                       \
        s_tState = START;      \
    } while (0);

bool search_msg_map_init(search_msg_map_t *ptThis, search_msg_map_cfg_t *ptCFG)
{
    enum {
        START
    };
    if ((NULL == ptCFG)) {
        if (    (NULL == ptThis) 
            ||  (NULL == ptCFG->ptReadByteEvent)
            ||  (NULL == ptCFG->ptMSGMap)
            ||  (NULL == ptCFG->ptQueue))
            {
                return false;
            }
    }
    this.chState = START;
    this.chMSGNumber = ptCFG->chMSGNumber;
    this.ptQueue = ptCFG->ptQueue;
    this.ptReadByteEvent = ptCFG->ptReadByteEvent;
    this.ptMSGMap = ptCFG->ptMSGMap;
    return true;
}

msg_t *search_msg_map(search_msg_map_t *ptThis)
{
    enum {
        START,
        INIT_CHECK_MSG,
        CHECK_MSG,
        MSG_HANLDER,
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
                const check_str_cfg_t c_tCheckMSGCFG = {(this.ptMSGMap[this.chMSGCount]).pchMessage, this.ptReadByteEvent};
                check_string_init(&this.ptCheckMSG, &c_tCheckMSGCFG);
            } while (0);
            this.chState = CHECK_MSG;
            //break;
        case CHECK_MSG:
            if (fsm_rt_cpl == check_string(&this.ptCheckMSG, &this.bIsRequestDrop)) {
                GET_ALL_PEEKED_BYTE(this.ptQueue);
                this.chState = MSG_HANLDER;
                return this.ptMSGMap;
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
        case MSG_HANLDER:
        // GOTO_MSG_HANLDER:
            if (fsm_rt_cpl == (this.ptMSGMap[this.chMSGCount]).fnHandler(&this.ptMSGMap[this.chMSGCount])) {
                this.chState = CHECK_MSG_NUMBER;
                TASK_RESET_FSM();
            } else {
                
                // goto GOTO_MSG_HANLDER;
            }
            break;
        default:
            return NULL;
            break;
    }
    return NULL;
}
