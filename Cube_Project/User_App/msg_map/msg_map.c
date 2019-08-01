#include "app_cfg.h"
#include "../queue/queue.h"
#include "../check_string/check_string.h"
#include "../check_use_peek/check_use_peek.h"
#include "../msg_map/msg_map.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#define this (*ptThis)
#define TASK_RESET_FSM()      \
    do {                      \
        this.chState = START; \
    } while (0);

bool check_use_peek_init(search_msg_map_t *ptThis, const msg_t *ptCFG)
{
    enum {
        START
    };
    if (   (NULL == ptCFG) 
        || (NULL == ptThis) 
        || (NULL == ptCFG->pTarget)
        || (NULL == ptCFG->pchMessage) 
        || (NULL == ptCFG->fnHandler))) {
        return false;
    }
    this.pchMessage=ptCFG->pchMessage;
    this.pTarget=ptCFG->pTarget;
    this.fnHandler=ptCFG->fnHandler;
    return true;
}

fsm_rt_t search_msg_map(search_msg_map_t *ptThis)
{
    enum {
        START,
        CHECK_MSG,
        DROP,
        CHECK_MSG_NUMBER,
        MSG_HANLDER
    };
    switch (this.chState) {
        case START:
            this.chVoteDropCount = 0;
            this.chWordsCount = 0;
            this.chState = CHECK_MSG;
            // break;
        case CHECK_MSG:
            do {
                bool bIsRequestDrop = false;
                RESET_PEEK_BYTE(this.ptQueue);
                if (fsm_rt_cpl == this.ptAgents[this.chWordsCount].fnCheckWords(
                                        this.ptAgents[this.chWordsCount].pTarget,
                                        &this.tReadByte, 
                                        &bIsRequestDrop)) {
                    GET_ALL_PEEKED_BYTE(this.ptQueue);
                    this.chState = MSG_HANLDER;
                    break;
                }
                if (bIsRequestDrop) {
                    this.chVoteDropCount++;
                }
                bIsRequestDrop = false;
            } while (0);
            this.chState = CHECK_MSG_NUMBER;
            //break;
        case CHECK_MSG_NUMBER:
            this.chWordsCount++;
            if (this.chWordsCount >= this.chAgentsNumber) {
                this.chWordsCount = 0;
                this.chState = DROP;
                //break;
            } else {
                this.chState = CHECK_MSG;
                break;
            }
        case DROP:
            if (this.chVoteDropCount >= this.chAgentsNumber) {
                do {
                    uint8_t chByteDrop;
                    DEQUEUE_BYTE(this.ptQueue, &chByteDrop);
                } while (0);
                RESET_PEEK_BYTE(this.ptQueue);
                this.chVoteDropCount = 0;
            }
            TASK_RESET_FSM();
            break;
        case MSG_HANLDER:
        this.ptAgentsHanlder[this.chWordsCount]
            if(fsm_rt_cpl=this.ptAgents[this.chWordsCount].fnHandler(this.ptAgents[this.chWordsCount]))
            {
                TASK_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}
