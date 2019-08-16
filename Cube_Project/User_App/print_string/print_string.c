
#include "app_cfg.h"
#include "../print_string/print_string.h"
#include <stdint.h>
#include <stdbool.h>
#include "../queue/queue.h"
#include "../uart/uart.h"
#define this (*ptThis)

#define TASK_STR_RESET_FSM()  \
    do {                      \
        this.chState = START; \
    } while (0);
#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
#ifndef PRINT_STR_OUTPUT_BYTE
#error No defined macro PRINT_STR_OUTPUT_BYTE(__TARGET,__BYTE) for output byte, please define one with prototype bool (*)(void* pTarget,uint8_t chByte);
#endif
#else
#ifndef PRINT_STR_OUTPUT_BYTE
#error No defined macro PRINT_STR_OUTPUT_BYTE(__BYTE) for output byte, please define one with prototype bool (*)(uint8_t chByte);
#endif
#endif

#define PRINT_STR_POOL_ITEM_TOTAL_SIZE sizeof(print_str_pool_item_t)

static print_str_pool_item_t *s_ptFreeList;

bool print_string_init(print_str_t *ptThis, const print_str_cfg_t *ptCFG)
{
    enum {
        START
    };
    if (   (NULL == ptCFG) 
        || (NULL == ptThis) 
        || (NULL == ptCFG->pTarget)
        || (NULL == ptCFG->fnPrintByte)) {
        return false;
    }
    this.chState = START;
    this.pchString = ptCFG->pchString;
    this.pTarget = ptCFG->pTarget;
#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
    this.fnPrintByte = ptCFG->fnPrintByte;
#endif
    return true;
}

fsm_rt_t print_string(print_str_t *ptThis)
{
    enum {
        START,
        PRINT_CHECK,
        PRINT_STR
    };
    if (NULL == ptThis || (this.fnPrintByte == NULL) || (NULL == this.pTarget)) {
        return fsm_rt_err;
    }
    switch (this.chState) {
        case START:
            this.chState = PRINT_CHECK;
            // break;
        case PRINT_CHECK:
            if ('\0' == *this.pchString) {
                TASK_STR_RESET_FSM();
                return fsm_rt_cpl;
            } else {
                this.chState = PRINT_STR;
            }
            // break;
        case PRINT_STR:
#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
            if (PRINT_STR_OUTPUT_BYTE(this.pTarget, *this.pchString)) {
                this.pchString++;
                this.chState = PRINT_CHECK;
            }
#else
            if (PRINT_STR_OUTPUT_BYTE(*this.pchString)) {
                this.pchString++;
                this.chState = PRINT_CHECK;
            }
#endif
            break;
        default:
            return fsm_rt_err;
            break;
    }
    return fsm_rt_on_going;
}

void print_str_pool_item_init(void)
{
    s_ptFreeList = NULL;
}

bool print_str_pool_add_heap(void *pTartget, uint16_t hwSize)
{
    uint8_t *ptThis = (uint8_t *)pTartget;
    if ((NULL == pTartget) || (hwSize < PRINT_STR_POOL_ITEM_TOTAL_SIZE)) {
        printf("memory block is too small, please at least %d\r\n", PRINT_STR_POOL_ITEM_TOTAL_SIZE);
        return false;
    } else {
        for (uint16_t hwSizeCounter = hwSize; PRINT_STR_POOL_ITEM_TOTAL_SIZE < hwSizeCounter; hwSizeCounter -= PRINT_STR_POOL_ITEM_TOTAL_SIZE) {
            print_str_pool_push((print_str_pool_item_t *)ptThis);
            ptThis = ptThis + PRINT_STR_POOL_ITEM_TOTAL_SIZE;
        }
        return true;
    }
}

print_str_t *print_str_pool_allocate(void)
{
    print_str_pool_item_t *ptThis;
    if (NULL == s_ptFreeList) {
        printf("pool is null\r\n");
        return NULL;
    }
    ptThis = s_ptFreeList;
    s_ptFreeList = s_ptFreeList->ptNext;
    this.ptNext = NULL;
    return (print_str_t *)this.chBuffer;
}

void print_str_pool_free(print_str_t *ptItem)
{
    if (ptItem != NULL) {
        print_str_pool_push((print_str_pool_item_t *)ptItem);
    }
}

void print_str_pool_push(print_str_pool_item_t *ptThis)  //入栈
{
    this.ptNext = s_ptFreeList;
    s_ptFreeList = ptThis;
}