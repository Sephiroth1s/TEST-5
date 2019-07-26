/*****************************************************************************
 *   Copyright(C)2009-2019 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

/*============================ INCLUDES ======================================*/
#include "../vsf_kernel_cfg.h"

#if VSF_USE_KERNEL_THREAD_MODE == ENABLED

#define __VSF_THREAD_CLASS_IMPLEMENT
#include "../vsf_kernel_common.h"
#include "./vsf_thread.h"


/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

SECTION("text.vsf.kernel.vsf_thread_get_cur")
vsf_thread_t *vsf_thread_get_cur(void)
{
    return (vsf_thread_t *)vsf_eda_get_cur();
}

SECTION("text.vsf.kernel.vsf_thread_ret")
void vsf_thread_ret(void)
{
    vsf_thread_t *thread_obj = vsf_thread_get_cur();
    class_internal(thread_obj, thread, vsf_thread_t);
    ASSERT(thread != NULL);
    longjmp(*(thread)->ret, 0);
}

SECTION("text.vsf.kernel.vsf_thread_wait")
vsf_evt_t vsf_thread_wait(void)
{
    vsf_thread_t *thread_obj = vsf_thread_get_cur();
    class_internal(thread_obj, thread, vsf_thread_t);
    vsf_evt_t curevt;
    jmp_buf pos;

    ASSERT(thread != NULL);
    thread->pos = &pos;
    curevt = setjmp(*thread->pos);
    //pthread_obj->current_evt = curevt;
    if (!curevt) {
        vsf_thread_ret();
    }
    return curevt;
}

SECTION("text.vsf.kernel.vsf_thread_wfe")
void vsf_thread_wfe(vsf_evt_t evt)
{
    vsf_thread_t *thread_obj = vsf_thread_get_cur();
    class_internal(thread_obj, thread, vsf_thread_t);
    vsf_evt_t curevt;
    jmp_buf pos;

    ASSERT(thread != NULL);
    thread->pos = &pos;
    curevt = setjmp(*thread->pos);
    //pthread_obj->current_evt = curevt;
    if (!curevt || (curevt != evt)) {
        vsf_thread_ret();
    }
}

SECTION("text.vsf.kernel.vsf_thread_sendevt")
void vsf_thread_sendevt(vsf_thread_t *thread, vsf_evt_t evt)
{
    ASSERT(thread != NULL);
    class_internal(thread, pthis, vsf_thread_t)
    vsf_eda_post_evt(&pthis->use_as__vsf_eda_t, evt);
}

SECTION("text.vsf.kernel.vsf_thread")
static void __vsf_thread_entry(void)
{
    vsf_thread_t *thread_obj = vsf_thread_get_cur();
    class_internal(thread_obj, thread, vsf_thread_t);
    thread->entry(thread_obj);

#if VSF_CFG_TIMER_EN
    vsf_teda_fini((vsf_teda_t *)thread_obj);
#else
    vsf_eda_fini((vsf_eda_t *)thread_obj);
#endif
    longjmp(*(thread)->ret, 0);
}

SECTION("text.vsf.kernel.vsf_thread")
static void __vsf_thread_evthandler(vsf_eda_t *eda, vsf_evt_t evt)
{
    class_internal((vsf_thread_t *)eda, thread, vsf_thread_t);
    jmp_buf ret;

    ASSERT(thread != NULL);

    thread->ret = &ret;
    if (!setjmp(ret)) {
        if (VSF_EVT_INIT == evt) {
            vsf_arch_set_stack((uint_fast32_t)(&thread->stack[(thread->stack_size>>3)]));
            __vsf_thread_entry();
        } else {
            longjmp(*thread->pos, evt);
        }
    }
}

SECTION("text.vsf.kernel.vsf_thread")
vsf_err_t vsf_thread_start(vsf_thread_t *thread, vsf_priority_t priority)
{
    class_internal(thread, pthis, vsf_thread_t)
    ASSERT(pthis != NULL);
    pthis->evthandler = __vsf_thread_evthandler;
#if VSF_CFG_TIMER_EN
    return vsf_teda_init(&pthis->use_as__vsf_teda_t, priority, true);
#else
    return vsf_eda_init(&pthis->use_as__vsf_eda_t, priority, true);
#endif
}

#if VSF_CFG_TIMER_EN
SECTION("text.vsf.kernel.vsf_thread_delay")
void vsf_thread_delay(uint_fast32_t tick)
{
    vsf_teda_set_timer(tick);
    vsf_thread_wfe(VSF_EVT_TIMER);
}
#endif

#if VSF_CFG_SYNC_EN

SECTION("text.vsf.kernel.vsf_thread_mutex")
static vsf_sync_reason_t __vsf_thread_wait_for_sync(vsf_sync_t *sync, int_fast32_t time_out)
{
    vsf_err_t err;
    vsf_sync_reason_t reason;

    err = vsf_eda_sync_decrease(sync, time_out);
    if (!err) { return VSF_SYNC_GET; }
    else if (err < 0) { return VSF_SYNC_FAIL; }
    //else {
        do {
            reason = vsf_eda_sync_get_reason(sync, vsf_thread_wait());
        } while (reason == VSF_SYNC_PENDING);
        return reason;
    //}
}

SECTION("text.vsf.kernel.vsf_thread_mutex")
vsf_sync_reason_t vsf_thread_mutex_enter(vsf_mutex_t *mtx, int_fast32_t timeout)
{
    return __vsf_thread_wait_for_sync(&mtx->use_as__vsf_sync_t, timeout);
}

SECTION("text.vsf.kernel.vsf_thread_mutex")
vsf_sync_reason_t vsf_thread_sem_pend(vsf_sem_t *sem, int_fast32_t timeout)
{
    return __vsf_thread_wait_for_sync(sem, timeout);
}

SECTION("text.vsf.kernel.vsf_thread_mutex")
vsf_err_t vsf_thread_mutex_leave(vsf_mutex_t *mtx)
{
    return vsf_eda_mutex_leave(mtx);
}

SECTION("text.vsf.kernel.vsf_thread_sem_post")
vsf_err_t vsf_thread_sem_post(vsf_sem_t *sem)
{
    return vsf_eda_sem_post(sem);
}

#if VSF_CFG_BMPEVT_EN

SECTION("text.vsf.kernel.vsf_thread_bmpevt_pend")
vsf_sync_reason_t vsf_thread_bmpevt_pend(
                    vsf_bmpevt_t *bmpevt,
                    vsf_bmpevt_pender_t *pender,
                    int_fast32_t timeout)
{
    vsf_sync_reason_t reason;
    vsf_err_t err;
    vsf_evt_t evt;

    err = vsf_eda_bmpevt_pend(bmpevt, pender, timeout);
    if (!err) { return VSF_SYNC_GET; }
    else if (err < 0) { return VSF_SYNC_FAIL; }
    else {
        while (1) {
            evt = vsf_thread_wait();
            reason = vsf_eda_bmpevt_poll(bmpevt, pender, evt);
            if (reason != VSF_SYNC_PENDING) {
                return reason;
            }
        }
    }
}
#endif
#endif      // VSF_CFG_SYNC_EN

#endif
