// SPDX-License-Identifier: GPL-2.0-or-later
#include "./app_cfg.h"
#ifndef __KEY_H__
#define __KEY_H__

/*============================ INCLUDES ======================================*/
#include "stm32f1xx_hal.h"
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
/*============================ TYPES =========================================*/
typedef enum {
    KEY_NULL = 0,
    KEY_DOWN,
    KEY_UP,
} key_evt_t;
typedef struct key_t {
    key_evt_t tEvent;
} key_t;
declare_class(key_service_t)
def_class(key_service_t,
    private_member(
        uint8_t chState;
        key_t *ptKeyEvent;
    )
)
end_def_class(key_service_t)

def_interface(i_key_service_t)
    bool     (*Init)    (key_service_t *ptThis);
    key_t    (*GetKey)  (key_service_t *ptThis);
    fsm_rt_t (*Task)    (key_service_t *ptThis);
    struct{
        fsm_rt_t (*Raising) (void);
        fsm_rt_t (*Falling) (void);
    }WaitKey;
end_def_interface(i_key_service_t)
/*============================ GLOBAL VARIABLES ==============================*/
extern const i_key_service_t KEY_SERVICE;
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

extern void key_init(void);
extern fsm_rt_t wait_raising_edge(void);
extern fsm_rt_t wait_falling_edge(void);
extern bool key_service_init(key_service_t *ptThis);
extern key_t key_service_get_key(key_service_t *ptThis);
extern fsm_rt_t key_service_task(key_service_t *ptThis);
#endif
/* EOF */

