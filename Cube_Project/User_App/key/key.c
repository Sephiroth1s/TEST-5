// SPDX-License-Identifier: GPL-2.0-or-later
#include "./app_cfg.h"
/*============================ INCLUDES ======================================*/
#include "stm32f1xx_hal.h"
#define __MSG_MAP_CLASS_IMPLEMENT
#include "./key.h"
#include "../queue/queue.h"
/*============================ MACROS ========================================*/
#ifndef this
    #define this (*ptThis)
#endif
#if VSF_USE_KEY_FILTER
    #define KEY_THRESHOLD 30
#elif
    #define KEY_THRESHOLD 20
#endif
/*============================ MACROFIED FUNCTIONS ===========================*/
#define IS_KEY1_DOWN()  (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5))
#define IS_KEY1_UP()    (GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5))
#define IS_KEY2_DOWN()  //todo
#define IS_KEY2_UP()    //todo
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

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

#define TASK_RESET_FSM()  do{ s_tState = START; } while(0)
fsm_rt_t wait_raising_edge(void)
{
    static enum {
        START = 0,
        CHECK_KEY,
        CHECK_HIGH
    }s_tState=START;
    static uint8_t s_chCnt;
    switch (s_tState) {
        case START:
            s_chCnt = 0;
            break;
        case CHECK_KEY:
            if (IS_KEY1_UP()) {
                s_chCnt++;
            #if VSF_USE_KEY_FILTER
                } else if (s_chCnt > 0) {
                s_chCnt--;
            }
            #else
            } else {
                s_chCnt = 0;
            }
            #endif
            s_tState = CHECK_HIGH;
            // break;
        case CHECK_HIGH:
            if (s_chCnt >= KEY_THRESHOLD) {
                uint8_t chByte = KEY_UP;
                //ENQUEUE_BYTE(this.ptQueue,chByte);
                TASK_RESET_FSM();
                return fsm_rt_cpl;
            } else {
                s_tState = CHECK_KEY;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}
fsm_rt_t wait_falling_edge(void)
{
    static enum {
        START = 0,
        CHECK_KEY,
        CHECK_HIGH
    }s_tState=START;
    static uint8_t s_chCnt;
    switch (s_tState) {
        case START:
            s_chCnt = 0;
            break;
        case CHECK_KEY:
            if (IS_KEY1_DOWN()) {
                s_chCnt++;
            #if VSF_USE_KEY_FILTER
                } else if (s_chCnt > 0) {
                s_chCnt--;
            }
            #else
            } else {
                s_chCnt = 0;
            }
            #endif
            s_tState = CHECK_HIGH;
            // break;
        case CHECK_HIGH:
            if (s_chCnt >= KEY_THRESHOLD) {
                uint8_t chByte = KEY_DOWN;
                //ENQUEUE_BYTE(this.ptQueue,chByte);
                TASK_RESET_FSM();
                return fsm_rt_cpl;
            } else {
                s_tState = CHECK_KEY;
            }
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

bool key_service_init(key_service_t *ptThis)
{

}
key_t key_service_get_key(key_service_t *ptThis)
{
    uint8_t chByte;
//    if(DEQUEUE_BYTE(this.ptQueue,&chByte)){
//        this.tKeyState.tEvent = chByte;
//        return this.tKeyState;
//    }
    
}
fsm_rt_t key_service_task(key_service_t *ptThis)
{
    
}
