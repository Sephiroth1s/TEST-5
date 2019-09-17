#include "app_cfg.h"
#include "../queue/queue.h"
#include "../check_string/check_string.h"
#define __CHECK_USE_PEEK_CLASS_IMPLEMENT
#include "./check_use_peek.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define this (*ptThis)
#define TASK_RESET_FSM()      \
    do {                      \
        this.chState = START; \
    } while (0);

#ifndef ASSERT
#   define ASSERT(...)
#endif

const i_check_use_peek_t CHECK_USE_PEEK = {
    .Init = &check_use_peek_init,
    .CheckUsePeek = &task_check_use_peek,
};

bool check_use_peek_init(check_use_peek_t *ptObj, const check_use_peek_cfg_t *ptCFG)
{
    /* initialise "this" (i.e. ptThis) to access class members */
    class_internal(ptObj, ptThis, check_use_peek_t);
    enum {
        START
    };
    ASSERT(NULL != ptObj && NULL != ptCFG);

    if (   (NULL == ptCFG->ptQueue)
        || (NULL == ptCFG->ptAgents) 
        || (NULL == ptCFG->ptAgents->pTarget) 
        || (NULL == ptCFG->ptAgents->fnCheckWords)) {
        return false;
    }
    this.chState = START;
    this.chAgentsNumber = ptCFG->chAgentsNumber;
    this.ptQueue = ptCFG->ptQueue;
    this.ptAgents = ptCFG->ptAgents;
    this.tReadByte.fnReadByte = (read_byte_t *)&peek_byte_queue;
    this.tReadByte.pTarget = ptCFG->ptQueue;
    return true;
}

fsm_rt_t task_check_use_peek(check_use_peek_t *ptObj)
{
    /* initialise "this" (i.e. ptThis) to access class members */
    class_internal(ptObj, ptThis, check_use_peek_t);
    enum {
        START,
        CHECK_WORDS,
        DROP,
        CHECK_WORDS_NUMBER
    };
    ASSERT(NULL != ptObj);
    
    switch (this.chState) {
        case START:
            this.chVoteDropCount = 0;
            this.chWordsCount = 0;
            this.chState = CHECK_WORDS;
            // break;
        case CHECK_WORDS:
            do {
                bool bIsRequestDrop = false;
                RESET_PEEK_BYTE(this.ptQueue);
                if (fsm_rt_cpl == this.ptAgents[this.chWordsCount].fnCheckWords(
                                        this.ptAgents[this.chWordsCount].pTarget,
                                        &this.tReadByte, 
                                        &bIsRequestDrop)) {
                    GET_ALL_PEEKED_BYTE(this.ptQueue);
                    TASK_RESET_FSM();
                    return fsm_rt_cpl;
                }
                if (bIsRequestDrop) {
                    this.chVoteDropCount++;
                }
                bIsRequestDrop = false;
            } while (0);
            this.chState = CHECK_WORDS_NUMBER;
            //break;
        case CHECK_WORDS_NUMBER:
            this.chWordsCount++;
            if (this.chWordsCount >= this.chAgentsNumber) {
                this.chWordsCount = 0;
                this.chState = DROP;
                //break;
            } else {
                this.chState = CHECK_WORDS;
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
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}
