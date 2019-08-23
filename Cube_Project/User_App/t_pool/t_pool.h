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

#define PRINT_STR_POOL_ITEM_SIZE(__TYPE) (sizeof(__TYPE))

#define EXTERN_POOL(__NAME, __TYPE)                                        \
    typedef __NAME##_pool_item_t __NAME##_pool_item_t;                     \
    union {                                                                \
        uint_fast8_t ptBuffer[PRINT_STR_POOL_ITEM_SIZE[__TYPE]];           \
        __NAME##_item_pool_t *ptNext;                                      \
    } ALIGN(__alignof__(__TYPE));                                          \
    extern void __NAME##_item_init(__NAME##_pool_item_t *ptPool);          \
    extern _TYPE *__NAME##_pool_allocate(__NAME##_pool_item_t *ptPool);    \
    extern bool __NAME##_pool_add_heap(__NAME##_pool_item_t *ptPool,       \
                                       uint8_t *pTarget, uint16_t hwSize); \
    extern void __NAME##_pool_free(__NAME##_pool_item_t *ptPool,           \
                                   __TYPE *ptItem);                        \
    static void __NAME##_pool_push(__NAME##_pool_item_t *ptPool);

#define DEF_POOL_EX(__NAME, __TYPE)                                          \
    typedef __NAME##_pool_item_t __NAME##_pool_item_t;                       \
    union {                                                                  \
        uint_fast8_t ptBuffer[PRINT_STR_POOL_ITEM_SIZE[__TYPE]];             \
        __NAME##_item_pool_t *ptNext;                                        \
    } ALIGN(__alignof__(__TYPE));                                            \
    extern void __NAME##_item_init(__NAME##_pool_item_t *ptPool);            \
    extern _TYPE *__NAME##_pool_allocate(__NAME##_pool_item_t *ptPool);      \
    extern bool __NAME##_pool_add_heap(__NAME##_pool_item_t *ptPool,         \
                                       uint8_t *pTarget, uint16_t hwSize);   \
    extern void __NAME##_pool_free(__NAME##_pool_item_t *ptPool,             \
                                   __TYPE *ptItem);                          \
    static void __NAME##_pool_push(__NAME##_pool_item_t *ptPool);            \
    void __NAME##_item_init(__NAME##_pool_item_t *ptPool) { ptPool = NULL; } \
                                                                             \
    _TYPE *__NAME##_pool_allocate(__NAME##_pool_item_t *ptPool)              \
    {                                                                        \
        __NAME##_pool_item_t *ptThis;                                        \
        if (NULL == ptPool) {                                                \
            return NULL;                                                     \
        }                                                                    \
        ptThis = ptPool;                                                     \
        ptPool = ptPool->ptNext;                                             \
        ptThis->ptNext = NULL;                                               \
        return (__TYPE *)ptThis->ptBuffer                                    \
    }                                                                        \
                                                                             \
    bool __NAME##_pool_add_heap(__NAME##_pool_item_t *ptPool,                \
                                uint8_t *pTarget, uint16_t hwSize)           \
    {                                                                        \
        uint_fast8_t *ptThis = (uint_fast8_t *)pTarget;                      \
        hwSize = hwSize / sizeof(uint_fast8_t);                              \
        if ((NULL == pTarget) ||                                             \
            (hwSize < PRINT_STR_POOL_ITEM_SIZE[__TYPE])) {                   \
            return false;                                                    \
        } else {                                                             \
            for (uint16_t hwSizeCounter = hwSize;                            \
                 PRINT_STR_POOL_ITEM_SIZE[__TYPE] < hwSizeCounter;           \
                 hwSizeCounter -= PRINT_STR_POOL_ITEM_SIZE[__TYPE])          \
                __NAME##_pool_push((__NAME##_pool_item_t *)ptThis);          \
            ptThis = ptThis + sizeof(__NAME##_pool_item_t);                  \
        }                                                                    \
    }                                                                        \
                                                                             \
    void __NAME##_pool_free(__NAME##_pool_item_t *ptPool, __TYPE *ptItem)    \
    {                                                                        \
        if (ptItem != NULL) {                                                \
            __NAME##_pool_push((__NAME##_pool_item_t *)ptItem);              \
        }                                                                    \
    }                                                                        \
                                                                             \
    void __NAME##_pool_push(__NAME##_pool_item_t *ptPool,                    \
                            __NAME##_pool_item_t *ptThis)                    \
    {                                                                        \
        ptThis->ptNext = ptPool;                                             \
        ptPool = ptThis;                                                     \
    }

/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

#endif