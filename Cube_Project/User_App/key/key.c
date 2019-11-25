// SPDX-License-Identifier: GPL-2.0-or-later
#include "./app_cfg.h"
/*============================ INCLUDES ======================================*/
#include "stm32f1xx_hal.h"
#define __KEY_CLASS_IMPLEMENT
#include "./key.h"
#include "../print_string/print_string.h"
/*============================ MACROS ========================================*/
#ifndef this
    #define this (*ptThis)
#endif
#if VSF_USE_KEY_FILTER
    #define KEY_THRESHOLD 30
#else
    #define KEY_THRESHOLD 40
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
#define ENQUEUE_KEY(__QUEUE, __OBJ) (enqueue_key(__QUEUE, __OBJ))
#define DEQUEUE_KEY(__QUEUE, __ADDR) (dequeue_key(__QUEUE, __ADDR))
#define IS_KEY_QUEUE_EMPTY(__QUEUE) (is_key_queue_empty(__QUEUE))
#ifndef IS_KEY1_DOWN()
#error No defined macro IS_KEY1_DOWN() for key pressed
#endif
#ifndef IS_KEY1_UP()
#error No defined macro IS_KEY1_UP()  for key raised
#endif
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
POOL(print_str) s_tPrintFreeList;
/*============================ LOCAL VARIABLES ===============================*/
static key_queue_t *s_ptQueue = NULL;
/*============================ PROTOTYPES ====================================*/
static bool init_key_queue(key_queue_t *ptObj, key_t *ptKeyEvent, uint16_t hwSize);
static bool is_key_queue_empty(key_queue_t *ptObj);
static bool is_key_queue_full(key_queue_t *ptObj);
static bool enqueue_key(key_queue_t *ptObj, key_t tKeyEvent);
static bool dequeue_key(key_queue_t *ptObj, key_t *ptKeyEvent);



static bool wait_raising_edge_init(wait_raising_edge_t *ptObj);
static bool wait_fallling_edge_init(wait_falling_edge_t *ptObj);
static fsm_rt_t wait_raising_edge(wait_raising_edge_t *ptObj);
static fsm_rt_t wait_falling_edge(wait_falling_edge_t *ptObj);
static bool key_service_init(key_service__t *ptThis);
static bool key_service_get_key(key_service_t *ptThis ,key_t *ptKey);
static fsm_rt_t key_service_task(key_service_t *ptThis);

const i_key_service_t KEY_SERVICE = {
    .Init = &key_service_init,
    .GetKey = &key_service_get_key,
    .Task = &key_service_task,
};

void key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin : PC5 */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

bool wait_raising_edge_init(wait_raising_edge_t *ptObj)
{
    class_internal(ptObj, ptThis, wait_raising_edge_t);
    enum {
        START
    };
    if (NULL==ptObj){
        return false;
    }
    this.chState = START;
    this.ptQueue = s_ptQueue;
    this.tHighCheck.chState = START;
    this.tLowCheck.chState = START;
    return true;
}

bool wait_fallling_edge_init(wait_falling_edge_t *ptObj)
{
    class_internal(ptObj, ptThis, wait_falling_edge_t);
    enum {
        START
    };
    if (NULL==ptObj){
        return false;
    }
    this.chState = START;
    this.ptQueue = s_ptQueue;
    this.tHighCheck.chState = START;
    this.tLowCheck.chState = START;
    return true;
}
bool key_service_init(key_service_t *ptObj)
{
    class_internal(ptObj, ptThis, key_service_t);
    enum {
        START
    };
    if (NULL == ptObj) {
        return false;
    }
    this.chState = START;
    s_ptQueue = this.tQueue;
    #error "队列未初始化"
    wait_raising_edge_init(&this.tWaitRaiseEdge);
    wait_fallling_edge_init(&this.tWaitFallEdge);
    return true;
}

#define TASK_RESET_FSM()  do{ this.chState = START; } while(0)
fsm_rt_t high_check(high_check_t *ptThis)
{
    enum {
        START = 0,
        CHECK_KEY,
        CHECK_HIGH
    };
    if(NULL == ptThis){
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chCnt = 0;
            this.chState = CHECK_KEY;
            // break;
        case CHECK_KEY:
            if (IS_KEY1_UP()) {
                this.chCnt++;
            #if VSF_USE_KEY_FILTER
                } else if (this.chCnt > 0) {
                this.chCnt--;
            }
            #else
            } else {
                this.chCnt = 0;
            }
            #endif
            this.chState = CHECK_HIGH;
            // break;
        case CHECK_HIGH:
            if (this.chCnt >= KEY_THRESHOLD) {
                TASK_RESET_FSM();
                return fsm_rt_cpl;
            } else {
                this.chState = CHECK_KEY;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

fsm_rt_t low_check(low_check_t *ptThis)
{
    enum {
        START = 0,
        CHECK_KEY,
        CHECK_HIGH
    };
    switch (this.chState) {
        case START:
            this.chCnt = 0;
            this.chState = CHECK_KEY;
            // break;
        case CHECK_KEY:
            if (IS_KEY1_DOWN()) {
                this.chCnt++;
            #if VSF_USE_KEY_FILTER
                } else if (this.chCnt > 0) {
                this.chCnt--;
            }
            #else
            } else {
                this.chCnt = 0;
            }
            #endif
            this.chState = CHECK_HIGH;
            // break;
        case CHECK_HIGH:
            if (this.chCnt >= KEY_THRESHOLD) {
                TASK_RESET_FSM();
                return fsm_rt_cpl;
            } else {
                this.chState = CHECK_KEY;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}
fsm_rt_t wait_raising_edge(wait_raising_edge_t *ptObj)
{
    class_internal(ptObj, ptThis, wait_raising_edge_t);
    enum {
        START = 0,
        CHECK_LOW,
        CHECK_HIGH
    };
    if(NULL == ptThis){
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.tHighCheck.chState = START;
            this.tLowCheck.chState = START;
            this.chState = CHECK_LOW;
            // break;
        case CHECK_LOW:
            if (fsm_rt_cpl != low_check(&this.tLowCheck)) {
                break;
            }
            this.chState = CHECK_HIGH;
            // break;
        case CHECK_HIGH:
            if (fsm_rt_cpl == high_check(&this.tHighCheck)) {
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

fsm_rt_t wait_falling_edge(wait_falling_edge_t *ptObj)
{
    class_internal(ptObj, ptThis, wait_falling_edge_t);
    enum {
        START = 0,
        CHECK_HIGH,
        CHECK_LOW,
    };
    switch (this.chState) {
        case START:
            this.chState = CHECK_HIGH;
            // break;
        case CHECK_HIGH:
            if (fsm_rt_cpl != high_check(&this.tHighCheck)) {
                break;
            }
            this.chState = CHECK_LOW;
            // break;
        case CHECK_LOW:
            if (fsm_rt_cpl == low_check(&this.tLowCheck)) {
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

bool key_service_get_key(key_service_t *ptObj ,key_t *ptKey)
{
    do {
        class_internal(ptObj, ptThis, key_service_t);
        if ((NULL == ptObj) || (NULL == ptKey)) {
            break;
        }
        if (DEQUEUE_KEY(this.tQueue, ptKey)) {
            return true;
        }
    } while (0);
    return false;
}
fsm_rt_t key_service_task(key_service_t *ptObj)
{
    class_internal(ptObj, ptThis, key_service_t);
    enum {
        START,
        WAIT_RAISING,
        WAIT_FALLING,
        USER_FUNCTION
    };
    if (NULL == ptObj) {
        return fsm_rt_err;
    }
    switch (this.chState)
    {
    case START:
        this.chState = WAIT_RAISING;
        // break;
    case WAIT_RAISING:
        if(fsm_rt_cpl==wait_raising_edge(&this.tWaitRaiseEdge)){
            key_t tKeyEvent = {.tEvent = KEY_UP};
            ENQUEUE_KEY(this.ptQueue, tKeyEvent);
            this.chState = USER_FUNCTION;
            goto GOTO_USER_FUNCTION;
        }
        // break;
    case WAIT_FALLING:
        if(fsm_rt_cpl==wait_falling_edge(&this.tWaitFallEdge)){
            key_t tKeyEvent = {.tEvent = KEY_DOWN};
            ENQUEUE_KEY(this.ptQueue, tKeyEvent);
            this.chState = USER_FUNCTION;
            goto GOTO_USER_FUNCTION;
        }
        this.chState = WAIT_RAISING;
        break;
    case USER_FUNCTION:
    GOTO_USER_FUNCTION:
        if(fsm_rt_cpl==this.fnKeyHandler(this.pTarget)){
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

bool enqueue_key(key_queue_t* ptObj, key_t tKeyEvent)
{
    class_internal(ptObj, ptThis, key_queue_t);
    if ((ptObj == NULL) || (is_key_queue_full(ptObj)||(this.ptBuffer==NULL))) {
        return false;
    }
    this.ptBuffer[this.hwTail] = tKeyEvent;
    this.hwTail++;
    if (this.hwTail >= this.hwSize) {
        this.hwTail = 0;
    }
    this.hwLength++;
    return true;
}

bool dequeue_key(key_queue_t* ptObj, key_t* tKeyEvent)
{
    class_internal(ptObj, ptThis, key_queue_t);
    if ((ptObj == NULL) || (is_key_queue_empty(ptObj))) {
        return false;
    }
    *tKeyEvent = this.ptBuffer[this.hwHead];
    this.hwHead++;
    if (this.hwHead >= this.hwSize) {
        this.hwHead = 0;
    }
    this.hwLength--;
    return true;
}

bool is_key_queue_full(key_queue_t* ptObj)
{
    class_internal(ptObj, ptThis, key_queue_t);
    if (ptObj == NULL) {
        return false;
    }
    if ((this.hwTail == this.hwHead) && (this.hwLength)) {
        return true;
    }
    return false;
}

bool is_key_queue_empty(key_queue_t* ptObj)
{
    class_internal(ptObj, ptThis, key_queue_t);
    if (ptObj == NULL) {
        return false;
    }
    if ((this.hwTail == this.hwHead) && !(this.hwLength)) {
        return true;
    }
    return false;
}

bool init_key_queue(key_queue_t* ptObj, key_t* ptBuffer, uint16_t hwSize)
{
    class_internal(ptObj, ptThis, key_queue_t);
    if ((ptObj == NULL) || (NULL == ptBuffer) || (0 == hwSize)) {
        return false;
    }
    this.hwHead = 0;
    this.hwTail = 0;
    this.ptBuffer = ptBuffer;
    this.hwSize = hwSize;
    this.hwLength = 0;
    return true;
}

