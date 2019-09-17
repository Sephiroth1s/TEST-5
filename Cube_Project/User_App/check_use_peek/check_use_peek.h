#include "app_cfg.h"
#ifndef __CHECK_USE_PEEK_H__
#define __CHECK_USE_PEEK_H__
#include "../queue/queue.h"
#include "../check_string/check_string.h"
#include <stdbool.h>
#include <stdint.h>
#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc.h"

typedef fsm_rt_t check_agent_handler_t(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop);

typedef struct {
    void *pTarget;
    check_agent_handler_t *fnCheckWords;
} check_agent_t;

#define __PLOOC_CLASS_USE_STRICT_TEMPLATE__
   
#if     defined(__CHECK_USE_PEEK_CLASS_IMPLEMENT)
#       define __PLOOC_CLASS_IMPLEMENT
#elif   defined(__CHECK_USE_PEEK_CLASS_INHERIT)
#       define __PLOOC_CLASS_INHERIT
#endif   

#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc_class.h"

declare_class(check_use_peek_t)

def_class(check_use_peek_t,
    private_member(
        uint8_t chState;
        uint8_t chAgentsNumber;
        uint8_t chVoteDropCount;
        uint8_t chWordsCount;
        byte_queue_t *ptQueue;
        read_byte_evt_handler_t tReadByte;
        check_agent_t *ptAgents; 
    )
)
end_def_class(check_use_peek_t)

typedef struct {
    uint8_t chAgentsNumber;
    byte_queue_t *ptQueue;
    check_agent_t *ptAgents;
} check_use_peek_cfg_t;

def_interface(i_check_use_peek_t)
    bool     (*Init)            (check_use_peek_t *ptObj, const check_use_peek_cfg_t *ptCFG);
    fsm_rt_t (*CheckUsePeek)    (check_use_peek_t *ptObj);
end_def_interface(i_check_use_peek_t)

extern const i_check_use_peek_t CHECK_USE_PEEK;

extern bool check_use_peek_init(check_use_peek_t *ptObj, const check_use_peek_cfg_t *ptCFG);
extern fsm_rt_t task_check_use_peek(check_use_peek_t *ptObj);
#endif
