#ifndef _USE_TEMPLATE_POOL_H
#define _USE_TEMPLATE_POOL_H



/*============================ INCLUDES ======================================*/
/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
#define POOL_INIT(__NAME, __POOL)                                                \
    __NAME##_pool_init((__POOL))

#define POOL_ALLOCATE(__NAME, __POOL)                                            \
    __NAME##_pool_allocate((__POOL))

#define POOL_FREE(__NAME, __POOL, __ITEM)                                        \
    __NAME##_pool_free((__POOL), (__ITEM))

#define POOL_ADD_HEAP(__NAME, __POOL, __BUFFER, __SIZE)                          \
    __NAME##_pool_add_heap((__POOL), (__BUFFER), (__SIZE))

#define POOL(__NAME) __NAME##_pool_t

#define DECLARE_POOL(__NAME, __TYPE)                                             \
    typedef union __NAME##_pool_item_t __NAME##_pool_item_t;                     \
    typedef struct __NAME##_pool_t __NAME##_pool_t;                              \
    extern bool __NAME##_pool_init(__NAME##_pool_t *ptPool);                     \
    extern __TYPE *__NAME##_pool_allocate(__NAME##_pool_t *ptPool);              \
    extern bool __NAME##_pool_add_heap(__NAME##_pool_t *ptPool,                  \
                                       uint8_t *pTarget, uint16_t hwSize);       \
    extern void __NAME##_pool_free(__NAME##_pool_t *ptPool, __TYPE *ptItem);     \
    static void __NAME##_pool_item_push(__NAME##_pool_t *ptPool,                 \
                                        __NAME##_pool_item_t *ptItem);

#define DEF_POOL(__NAME, __TYPE)                                                 \
    union __NAME##_pool_item_t {                                                 \
        __TYPE tItem;                                                            \
        __NAME##_pool_item_t *ptNext;                                            \
    } ALIGN(__alignof__(__TYPE));                                                \
    struct __NAME##_pool_t {                                                     \
        __NAME##_pool_item_t *ptFreeList;                                        \
    };

#define IMPLEMENT_POOL(__NAME, __TYPE)                                           \
    bool __NAME##_pool_init(__NAME##_pool_t *ptPool)                             \
    {                                                                            \
        if (NULL == ptPool) {                                                    \
            return false;                                                        \
        }                                                                        \
        ptPool->ptFreeList = NULL;                                               \
        return true;                                                             \
    }                                                                            \
                                                                                 \
    __TYPE *__NAME##_pool_allocate(__NAME##_pool_t *ptPool)                      \
    {                                                                            \
        __NAME##_pool_item_t *ptItem;                                            \
        if ((NULL == ptPool) || (NULL == ptPool->ptFreeList)) {                  \
            return NULL;                                                         \
        }                                                                        \
        ptItem = ptPool->ptFreeList;                                             \
        ptPool = (__NAME##_pool_t *)ptItem->ptNext;                              \
        ptItem->ptNext = NULL;                                                   \
        return &ptItem->tItem;                                                   \
    }                                                                            \
                                                                                 \
    bool __NAME##_pool_add_heap(__NAME##_pool_t *ptPool, uint8_t *pTarget,       \
                                uint16_t hwSize)                                 \
    {                                                                            \
        uint_fast8_t *ptItem = (uint_fast8_t *)pTarget;                          \
        uint16_t hwTypeSize;                                                     \
        hwTypeSize = ((sizeof(__TYPE) + sizeof(uint_fast8_t) - 1) /              \
                      sizeof(uint_fast8_t));                                     \
        if ((NULL == pTarget) || (hwSize < hwTypeSize)) {                        \
            return false;                                                        \
        } else {                                                                 \
            for (uint16_t hwSizeCounter = hwSize / sizeof(uint_fast8_t);         \
                 hwTypeSize < hwSizeCounter; hwSizeCounter -= hwTypeSize) {      \
                __NAME##_pool_item_push(ptPool, (__NAME##_pool_item_t *)ptItem); \
                ptItem = ptItem + hwTypeSize;                                    \
            }                                                                    \
            return true;                                                         \
        }                                                                        \
    }                                                                            \
                                                                                 \
    void __NAME##_pool_free(__NAME##_pool_t *ptPool, __TYPE *ptItem)             \
    {                                                                            \
        if ((NULL != ptPool) && (NULL != ptItem)) {                              \
            __NAME##_pool_item_push(ptPool, (__NAME##_pool_item_t *)ptItem);     \
        }                                                                        \
    }                                                                            \
                                                                                 \
    void __NAME##_pool_item_push(__NAME##_pool_t *ptPool,                        \
                                 __NAME##_pool_item_t *ptItem)                   \
    {                                                                            \
        ptItem->ptNext = ptPool->ptFreeList;                                     \
        ptPool->ptFreeList = ptItem;                                             \
    }

/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

#endif