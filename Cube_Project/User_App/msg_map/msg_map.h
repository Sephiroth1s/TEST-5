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
    uint8_t chMSGNumber;
    byte_queue_t *ptQueue;
    msg_t *ptMSGMap;
} search_msg_map_cfg_t;

typedef struct {
    uint8_t chState;
    uint8_t chMSGNumber;
    uint8_t chVoteDropCount;
    uint8_t chMSGCount;
    bool bIsRequestDrop;
    byte_queue_t *ptQueue;
    read_byte_evt_handler_t ptReadByteEvent;
    check_str_t tCheckMSG;
    msg_t *ptMSGMap;
} search_msg_map_t;

extern bool search_msg_map_init(search_msg_map_t *ptThis, search_msg_map_cfg_t *ptCFG);
extern msg_t *search_msg_map(search_msg_map_t *ptThis);
extern void msg_map_hanlder(msg_t*ptThis);

#endif