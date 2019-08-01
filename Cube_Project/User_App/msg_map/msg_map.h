#include "app_cfg.h"
#ifndef __MSG_MAP_H__
#define __MSG_MAP_H__
#include "../queue/queue.h"
#include "../check_string/check_string.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct _msg_t msg_t;
//! 消息的处理函数
typedef fsm_rt_t msg_hanlder_t (msg_t *ptMSG);
//! 消息（要处理的数据以及处理该数据的方法）
struct _msg_t {
        uint8_t *pchMessage;   
        void *pTarget;        
        msg_hanlder_t *fnHandler;
};

typedef struct {
    uint8_t chState;
    uint8_t chAgentsNumber;
    uint8_t chVoteDropCount;
    uint8_t chWordsCount;
    byte_queue_t *ptQueue;
    read_byte_evt_handler_t tReadByte;
    check_agent_t *ptAgents;
    msg_t *ptAgentsHanlder;
} search_msg_map_t;

#endif