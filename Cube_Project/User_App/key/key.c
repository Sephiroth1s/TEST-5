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
#define IS_KEY1_DOWN()  (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5))
#define IS_KEY1_UP()    (GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5))
#define IS_KEY2_DOWN()  //todo
#define IS_KEY2_UP()    //todo
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
POOL(print_str) s_tPrintFreeList;
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
static bool init_key_queue(key_queue_t *ptObj, key_t *ptKeyEvent, uint16_t hwSize);
static bool is_key_queue_empty(key_queue_t *ptObj);
static bool is_key_queue_full(key_queue_t *ptObj);
static bool enqueue_key(key_queue_t *ptObj, key_t tKeyEvent);
static bool dequeue_key(key_queue_t *ptObj, key_t *ptKeyEvent);
static bool check_edge_init(wait_raising_edge_t *ptRaiseObj, wait_falling_edge_t *ptFallObj, key_queue_t *ptQueue);
static fsm_rt_t wait_raising_edge(wait_raising_edge_t *ptObj);
static fsm_rt_t wait_falling_edge(wait_falling_edge_t *ptObj);
static bool key_service_init(key_service_t *ptObj, key_service_cfg_t *ptCFG);
static key_t key_service_get_key(key_service_t *ptObj);
static fsm_rt_t key_service_task(key_service_t *ptObj);

const i_key_service_t KEY_SERVICE = {
    .Init = &key_service_init,
    .GetKey = &key_service_get_key,
    .Task = &key_service_task,
    .KeyQueue.Init = &init_key_queue,
    .KeyQueue.Enqueue = &enqueue_key,
    .KeyQueue.Dequeue = &dequeue_key,
    .WaitKey.Init = &check_edge_init,
    .WaitKey.Raising = &wait_raising_edge,
    .WaitKey.Falling = &wait_falling_edge,
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
bool check_edge_init(wait_raising_edge_t *ptRaiseObj,wait_falling_edge_t *ptFallObj,key_queue_t *ptQueue)
{
    enum {
        START
    };
    if((NULL==ptRaiseObj)||(NULL==ptFallObj)){
        return false;
    }
    ptFallObj->chState = START;
    ptFallObj->ptQueue = ptQueue;
    ptRaiseObj->chState = START;
    ptRaiseObj->ptQueue = ptQueue;
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
                key_t tKeyEvent = {.tEvent = KEY_UP};
                ENQUEUE_KEY(this.ptQueue, tKeyEvent);
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
            this.tLowCheck.chState = START;
            this.tHighCheck.chState = START;
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
                key_t tKeyEvent = {.tEvent = KEY_DOWN};
                ENQUEUE_KEY(this.ptQueue, tKeyEvent);
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

bool key_service_init(key_service_t *ptObj, key_service_cfg_t *ptCFG)
{
    class_internal(ptObj, ptThis, key_service_t);
    enum {
        START
    };
    if (NULL == ptObj) {
        return false;
    }
    this.chState = START;
    this.pOutputTarget = ptCFG->pOutputTarget;
    this.ptQueue = ptCFG->ptQueue;
    return true;
}
key_t key_service_get_key(key_service_t *ptObj)
{
    class_internal(ptObj, ptThis, key_service_t);
    if(NULL==ptObj){
        this.tKeyEvent.tEvent = KEY_NULL;
        return this.tKeyEvent;
    }
    if (DEQUEUE_KEY(this.ptQueue, &this.tKeyEvent)) {
        return this.tKeyEvent;
    }
    this.tKeyEvent.tEvent = KEY_NULL;
    return this.tKeyEvent;
}

fsm_rt_t key_service_task(key_service_t *ptObj)
{
    class_internal(ptObj, ptThis, key_service_t);
    enum {
        START,
        GET_KEY,
        INIT_PRINT_KEY_UP,
        PRINT_KEY_UP,
        INIT_PRINT_KEY_DOWN,
        PRINT_KEY_DOWN,
    };
    if (NULL == ptObj) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chState = GET_KEY;
            // break;
        case GET_KEY:
            KEY_SERVICE.GetKey(ptThis);
            if (KEY_UP == this.tKeyEvent.tEvent) {
                this.chState = INIT_PRINT_KEY_UP;
                goto GOTO_INIT_PRINT_KEY_UP;
            }
            if (KEY_DOWN == this.tKeyEvent.tEvent) {
                this.chState = INIT_PRINT_KEY_DOWN;
                goto GOTO_INIT_PRINT_KEY_DOWN;
            }
            break;
        case INIT_PRINT_KEY_UP:
        GOTO_INIT_PRINT_KEY_UP:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (NULL == this.ptPrintStr) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {"KEY1 UP\r\n",
                                                this.pOutputTarget};
                PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
            } while (0);
            this.chState = PRINT_KEY_UP;
            // break;
        case PRINT_KEY_UP:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
                TASK_RESET_FSM();
                return fsm_rt_cpl;
            }
            break;
        case INIT_PRINT_KEY_DOWN:
        GOTO_INIT_PRINT_KEY_DOWN:
            this.ptPrintStr = POOL_ALLOCATE(print_str, &s_tPrintFreeList);
            if (NULL == this.ptPrintStr) {
                break;
            }
            do {
                const print_str_cfg_t c_tCFG = {"KEY1 DOWN\r\n",
                                                this.pOutputTarget};
                PRINT_STRING.Init(this.ptPrintStr, &c_tCFG);
            } while (0);
            this.chState = PRINT_KEY_DOWN;
            // break;
        case PRINT_KEY_DOWN:
            if (fsm_rt_cpl == PRINT_STRING.Print(this.ptPrintStr)) {
                POOL_FREE(print_str, &s_tPrintFreeList, this.ptPrintStr);
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

