
#include "app_cfg.h"
#include "../check_string/check_string.h"
#include <stdbool.h>
#include <stdint.h>
#include "../t_pool/t_pool.h"

#define this (*ptThis)
#define TASK_STR_RESET_FSM()  \
    do {                      \
        this.chState = START; \
    } while (0);

IMPLEMENT_POOL(check_str, check_str_t);
bool check_string_init(check_str_t *ptThis, const check_str_cfg_t *ptCFG)
{
    enum {
        START
    };
    if ((NULL == ptThis) || (NULL == ptCFG) || (NULL == ptCFG->ptReadByteEvent->fnReadByte)) {
        return false;
    }
    this.chState = START;
    this.pchOriginStr = ptCFG->pchString;
    this.pchString = ptCFG->pchString;
    this.ptReadByteEvent = ptCFG->ptReadByteEvent;
    return true;
}

fsm_rt_t check_string(check_str_t *ptThis, bool *pbIsRequestDrop)
{
    enum {
        START,
        CHECK_END,
        READ_CHAR,
        CHECK_WORLD
    };
    if (NULL == ptThis) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            *pbIsRequestDrop = false;
            this.pchString = this.pchOriginStr;
            this.chState = CHECK_END;
            // break;
        case CHECK_END:
        GOTO_CHECK_END:
            if (*this.pchString == '\0') {
                TASK_STR_RESET_FSM();
                return fsm_rt_cpl;
            } else {
                this.chState = READ_CHAR;
                printf("no check char end\r\n");
            }
            // break;
        case READ_CHAR:
            printf("readchar-\r\n");
            if (this.ptReadByteEvent->fnReadByte(this.ptReadByteEvent->pTarget, &this.chCurrentByte)) {
                printf("read_char successful\r\n");
                this.chState = CHECK_WORLD;
                // break;
            } else {
                printf("read char error\r\n");
                TASK_STR_RESET_FSM();
                break;
            }
        case CHECK_WORLD:
            printf("pchString:%c\tchCurrentByte%c\r\n",*this.pchString,this.chCurrentByte);
            if (*this.pchString == this.chCurrentByte) {
                printf("1pchString:%c\tchCurrentByte%c\r\n",*this.pchString,this.chCurrentByte);
                this.pchString++;
                this.chState = CHECK_END;
                goto GOTO_CHECK_END;
            } else {
                printf("check char errot\r\n");
                printf("check char errot ptThis%p\r\n",ptThis);
                printf("3pchString:%c\tchCurrentByte%c\r\n",*this.pchString,this.chCurrentByte);
                *pbIsRequestDrop = true;
                TASK_STR_RESET_FSM();
                break;
            }
            // break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}
