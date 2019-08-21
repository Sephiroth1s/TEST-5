#ifndef _USE_TEMPLATE_POOL_H
#define _USE_TEMPLATE_POOL_H



/*============================ INCLUDES ======================================*/
/*============================ MACROS ========================================*/
#define END_DEF_POOL(__NAME)

#define END_EXTERN_POOL(__NAME)
/*============================ MACROFIED FUNCTIONS ===========================*/
#define POOL_ITEM_INIT(__NAME, __POOL) \
    __NAME##_pool_item_init((__POOL))

#define POOL_ALLOCATE(__NAME, __POOL) \
    __NAME##_pool_allocate((__POOL))

#define POOL_FREE(__NAME, __POOL, __ITEM) \
    __NAME##_pool_free((__POOL), (__ITEM))

#define POOL_ADD_HEAP(_NAME, __POOL, __BUFFER, __SIZE) \
    __NAME##_pool_add_heap((__POOL), (__BUFFER), (__SIZE))

#define POOL(__NAME) __NAME##_item_pool_t

#define EXTERN_POOL(__NAME, __TYPE, __PTR_TYPE) \
    DECLARE_CLASS(__NAME##_item_pool_t)         \
    EXTERN_CLASS(__NAME##_item_pool_t)          \
    __TYPE *PtBuffer;                           \
    __PTR_TYPE *ptNext\
END_EXTERN_CLASS(__NAME##_item_pool_t)\

/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

#endif