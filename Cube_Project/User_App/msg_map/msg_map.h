#include "app_cfg.h"
#ifndef __MSG_MAP_H__
#define __MSG_MAP_H__
#include "../queue/queue.h"
#include "../check_string/check_string.h"
#include <stdbool.h>
#include <stdint.h>
#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc.h"

typedef struct _msg_t msg_t;
//! 消息的处理函数
typedef void msg_hanlder_t (msg_t *ptMSG);
//! 消息（要处理的数据以及处理该数据的方法）
struct _msg_t {
    uint8_t *pchMessage;   
    void *pTarget;        
    msg_hanlder_t *fnHandler;
};

#define __PLOOC_CLASS_USE_STRICT_TEMPLATE__
   
#if     defined(__MSG_MAP_CLASS_IMPLEMENT)
#       define __PLOOC_CLASS_IMPLEMENT
#elif   defined(__MSG_MAP_CLASS_INHERIT)
#       define __PLOOC_CLASS_INHERIT
#endif   

#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc_class.h"

declare_class(check_msg_map_t)

def_class(check_msg_map_t,
    private_member(
        uint8_t chState;
        uint8_t chMSGNumber;
        uint8_t chVoteDropCount;
        uint8_t chMSGCount;
        bool bIsRequestDrop;
        implement(byte_queue_t*)
        implement(check_str_t)
        implement(msg_t*)
    )
)
end_def_class(check_msg_map_t)

typedef struct {
    uint8_t chMSGNumber;
    implement(byte_queue_t*)
    implement(msg_t*)
} check_msg_map_cfg_t;


def_interface(i_check_msg_map_t)
    bool    (*Init) (check_msg_map_t *ptObj, check_msg_map_cfg_t *ptCFG);
    fsm_rt_t (*Check) (void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop);
end_def_interface(i_check_msg_map_t)

extern const i_check_msg_map_t CHECK_MSG_MAP;

extern bool check_msg_map_init(check_msg_map_t *ptObj, check_msg_map_cfg_t *ptCFG);
extern fsm_rt_t check_msg_map(void *pTarget, read_byte_evt_handler_t *ptReadByte, bool *pbRequestDrop);

#endif