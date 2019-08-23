#include "app_cfg.h"
#ifndef __PRINT_STRING_H__
#define __PRINT_STRING_H__
#include <stdint.h>
#include <stdbool.h>

typedef bool print_byte_t(void *, uint8_t);

typedef struct {
    uint8_t chState;
    uint8_t *pchString;
    void *pTarget;
#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
    print_byte_t *fnPrintByte;
#endif
} print_str_t;

typedef struct {
    uint8_t *pchString;
    void *pTarget;
#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
    print_byte_t *fnPrintByte;
#endif
} print_str_cfg_t;

#define PRINT_STR_POOL_ITEM_SIZE sizeof(print_str_t)

typedef union print_str_pool_item_t print_str_pool_item_t;  
union print_str_pool_item_t{  
	uint_fast8_t chBuffer[PRINT_STR_POOL_ITEM_SIZE];   //!< 节点的数据区域，可以是任何内容  
    print_str_pool_item_t *ptNext;
}ALIGN(__alignof__(print_str_t));  

static void print_str_pool_push(print_str_pool_item_t *ptThis);

extern void print_str_pool_item_init(void);
extern print_str_t *print_str_pool_allocate(void);
extern void print_str_pool_free(print_str_t *ptItem);
extern bool print_str_pool_add_heap(uint8_t *pTartget, uint16_t hwSize);

extern fsm_rt_t print_string(print_str_t *ptThis);
extern bool print_string_init(print_str_t *ptThis, const print_str_cfg_t *ptCFG);
#endif

   
   