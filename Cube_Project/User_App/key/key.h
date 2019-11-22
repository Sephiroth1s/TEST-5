// SPDX-License-Identifier: GPL-2.0-or-later
#include "./app_cfg.h"
#ifndef __KEY_H__
#define __KEY_H__

/*============================ INCLUDES ======================================*/
#include "stm32f1xx_hal.h"
#include "../print_string/print_string.h"
#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc.h"
/*============================ MACROS ========================================*/
#define __PLOOC_CLASS_USE_STRICT_TEMPLATE__
   
#if     defined(__KEY_CLASS_IMPLEMENT)
#       define __PLOOC_CLASS_IMPLEMENT
#elif   defined(__KEY_CLASS_INHERIT)
#       define __PLOOC_CLASS_INHERIT
#endif   
#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc_class.h"
/*============================ MACROFIED FUNCTIONS ===========================*/
#define ENQUEUE_KEY(__QUEUE, __OBJ) (enqueue_key(__QUEUE, __OBJ))
#define DEQUEUE_KEY(__QUEUE, __ADDR) (dequeue_key(__QUEUE, __ADDR))
#define INIT_KEY_QUEUE(__QUEUE, __SIZE)                        \
    do {                                                       \
        static key_t s_tBuffer[(__SIZE)];                      \
        init_key_queue(__QUEUE, s_tBuffer, sizeof(s_tBuffer)); \
    } while (0)
/*============================ TYPES =========================================*/

typedef enum {
    KEY_NULL = 0,
    KEY_DOWN,
    KEY_UP,
} key_evt_t;
typedef struct key_t {
    key_evt_t tEvent;
} key_t;

 declare_class(key_queue_t)
 def_class(key_queue_t,
     private_member(
         key_t* ptBuffer;
         uint16_t hwSize;
         uint16_t hwHead;
         uint16_t hwTail;
         uint16_t hwLength;
     )
 )
end_def_class(key_queue_t)

typedef struct {
    uint8_t chState;
    key_queue_t* ptQueue;
    uint8_t chCnt;
} wait_raising_edge_t;

typedef struct {
    uint8_t chState;
    key_queue_t* ptQueue;
    uint8_t chCnt;
} wait_falling_edge_t;

declare_class(key_service_t)
def_class(key_service_t,
    private_member(
        uint8_t chState;
        void *pOutputTarget;
        print_str_t *ptPrintStr;
        key_queue_t *ptQueue;
        key_t tKeyEvent;
    )
)
end_def_class(key_service_t)
typedef struct {
    void *pOutputTarget;
    key_queue_t *ptQueue;
}key_service_cfg_t;

def_interface(i_key_service_t)
    bool     (*Init)    (key_service_t *ptObj, key_service_cfg_t *ptCFG);
    key_t    (*GetKey)  (key_service_t *ptObj);
    fsm_rt_t (*Task)    (key_service_t *ptObj);
    struct{
        fsm_rt_t (*Raising) (wait_raising_edge_t *ptObj);
        fsm_rt_t (*Falling) (wait_falling_edge_t *ptObj);
    }WaitKey;
end_def_interface(i_key_service_t)
/*============================ GLOBAL VARIABLES ==============================*/
extern const i_key_service_t KEY_SERVICE;
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

extern void key_init(void);
extern bool enqueue_key(key_queue_t* ptObj, key_t tKeyEvent);
extern bool dequeue_key(key_queue_t* ptObj, key_t* ptKeyEvent);
extern bool init_key_queue(key_queue_t* ptObj, key_t* ptKeyEvent, uint16_t hwSize);
#endif
/* EOF */

