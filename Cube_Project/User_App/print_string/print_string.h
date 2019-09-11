#include "app_cfg.h"
#ifndef __PRINT_STRING_H__
#define __PRINT_STRING_H__
#include <stdint.h>
#include <stdbool.h>
#include "../t_pool/t_pool.h"
#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc.h"

typedef bool print_byte_t(void *, uint8_t);
typedef print_byte_t* fn_print_byte_t;

#define __PLOOC_CLASS_USE_STRICT_TEMPLATE__
   
#if     defined(__PRINT_STR_CLASS_IMPLEMENT)
#       define __PLOOC_CLASS_IMPLEMENT
#elif   defined(__PRINT_STR_CLASS_INHERIT)
#       define __PLOOC_CLASS_INHERIT
#endif   

#include "../../Vsf/release/kernel/beta/vsf/utilities/3rd-party/PLOOC/plooc_class.h"

declare_class(print_str_t)

def_class(print_str_t,
    private_member(
        uint8_t chState;
        uint8_t *pchString;
        void *pTarget;
#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
        implement(fn_print_byte_t);
#endif
    )
)
end_def_class(print_str_t)

typedef struct {
    uint8_t *pchString;
    void *pTarget;
#ifdef PRINT_STR_CFG_USE_FUNCTION_POINTER
    implement(fn_print_byte_t);
#endif
} print_str_cfg_t;

def_interface(i_print_str_t) 
    bool     (*Init)    (print_str_t *ptObj, print_str_cfg_t *ptCFG);
    fsm_rt_t (*Print)   (print_str_t *ptObj);
end_def_interface(i_print_str_t)

DECLARE_POOL(print_str, print_str_t);
DEF_POOL(print_str, print_str_t);

extern const i_print_str_t PRINT_STRING;

extern fsm_rt_t print_string(print_str_t *ptObj);
extern bool print_string_init(print_str_t *ptObj, const print_str_cfg_t *ptCFG);
#endif

