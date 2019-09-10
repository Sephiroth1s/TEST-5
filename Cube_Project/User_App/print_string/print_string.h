#include "app_cfg.h"
#ifndef __PRINT_STRING_H__
#define __PRINT_STRING_H__
#include <stdint.h>
#include <stdbool.h>
#include "../t_pool/t_pool.h"

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

DECLARE_POOL(print_str, print_str_t);
DEF_POOL(print_str, print_str_t);

extern fsm_rt_t print_string(print_str_t *ptThis);
extern bool print_string_init(print_str_t *ptThis, const print_str_cfg_t *ptCFG);
#endif
declare_class(print_str_t)
def_class(print_str_t,
private_member(

)
protected_member(

)
)
end_def_class(print_str_t)

def_interface(i_print_str_t)

end_def_interface(i_print_str_t)